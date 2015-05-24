#include "amulet.h"

// Cull face

void am_cull_face_node::render(am_render_state *rstate) {
    am_cull_face_state old_state = rstate->active_cull_face_state;
    switch (mode) {
        case AM_CULL_FACE_MODE_FRONT:
            rstate->active_cull_face_state.set(true, AM_FACE_WIND_CCW, AM_CULL_FACE_FRONT);
            break;
        case AM_CULL_FACE_MODE_BACK:
            rstate->active_cull_face_state.set(true, AM_FACE_WIND_CCW, AM_CULL_FACE_BACK);
            break;
        case AM_CULL_FACE_MODE_NONE:
            rstate->active_cull_face_state.set(false, AM_FACE_WIND_CCW, AM_CULL_FACE_BACK);
            break;
    }
    render_children(rstate);
    rstate->active_cull_face_state.restore(&old_state);
}

int am_create_cull_face_node(lua_State *L) {
    am_check_nargs(L, 2);
    am_cull_face_node *node = am_new_userdata(L, am_cull_face_node);
    am_set_scene_node_child(L, node);
    node->mode = am_get_enum(L, am_cull_face_mode, 2);
    return 1;
}

static void register_cull_face_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_metatable(L, "cull_face", MT_am_cull_face_node, MT_am_scene_node);
}

// Cull sphere

void am_cull_sphere_node::render(am_render_state *rstate) {
    glm::mat4 matrix = glm::mat4(1.0f);
    for (int i = 0; i < num_names; i++) {
        am_program_param_name_slot *slot = &am_param_name_map[names[i]];
        am_program_param_value *param = &slot->value;
        if (param->type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
            glm::mat4 *m = (glm::mat4*)&param->value.m4[0];
            matrix = matrix * *m;
        } else {
            am_log1("WARNING: matrix '%s' is not a mat4 in cull_sphere node (node will be culled)", slot->name);
            return;
        }
    }
    if (am_sphere_visible(matrix, center, radius)) {
        render_children(rstate);
    }
}

int am_create_cull_sphere_node(lua_State *L) {
    int nargs = am_check_nargs(L, 3);
    if (lua_type(L, 2) != LUA_TSTRING) return luaL_error(L, "expecting a string in position 2");
    am_cull_sphere_node *node = am_new_userdata(L, am_cull_sphere_node);
    am_set_scene_node_child(L, node);
    node->names[0] = am_lookup_param_name(L, 2);
    node->num_names = 1;
    int i = 3;
    while (i < nargs && lua_type(L, i) == LUA_TSTRING && node->num_names < AM_MAX_CULL_SPHERE_NAMES) {
        node->names[node->num_names] = am_lookup_param_name(L, i);
        node->num_names++;
        i++;
    }
    if (i <= nargs) {
        node->radius = luaL_checknumber(L, i);
        i++;
    } else {
        return luaL_error(L, "expecting radius in position %d", i);
    }
    if (i <= nargs) {
        node->center = am_get_userdata(L, am_vec3, i)->v;
        i++;
    } else {
        node->center = glm::vec3(0.0f);
    }
    return 1;
}

static void get_cull_sphere_radius(lua_State *L, void *obj) {
    am_cull_sphere_node *node = (am_cull_sphere_node*)obj;
    lua_pushnumber(L, node->radius);
}

static void set_cull_sphere_radius(lua_State *L, void *obj) {
    am_cull_sphere_node *node = (am_cull_sphere_node*)obj;
    node->radius = luaL_checknumber(L, 3);
}

static am_property cull_sphere_radius_property = {get_cull_sphere_radius, set_cull_sphere_radius};

static void get_cull_sphere_center(lua_State *L, void *obj) {
    am_cull_sphere_node *node = (am_cull_sphere_node*)obj;
    am_vec3 *center = am_new_userdata(L, am_vec3);
    center->v = node->center;
}

static void set_cull_sphere_center(lua_State *L, void *obj) {
    am_cull_sphere_node *node = (am_cull_sphere_node*)obj;
    node->center = am_get_userdata(L, am_vec3, 3)->v;
}

static am_property cull_sphere_center_property = {get_cull_sphere_center, set_cull_sphere_center};

static void register_cull_sphere_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "radius", &cull_sphere_radius_property);
    am_register_property(L, "center", &cull_sphere_center_property);

    am_register_metatable(L, "cull_sphere", MT_am_cull_sphere_node, MT_am_scene_node);
}

// Module init

void am_open_culling_module(lua_State *L) {
    am_enum_value cull_face_enum[] = {
        {"front", AM_CULL_FACE_MODE_FRONT},
        {"cw", AM_CULL_FACE_MODE_FRONT},
        {"back", AM_CULL_FACE_MODE_BACK},
        {"ccw", AM_CULL_FACE_MODE_BACK},
        {"none", AM_CULL_FACE_MODE_NONE},
        {NULL, 0}
    };
    am_register_enum(L, ENUM_am_cull_face_mode, cull_face_enum);
    register_cull_face_node_mt(L);
    register_cull_sphere_node_mt(L);
}
