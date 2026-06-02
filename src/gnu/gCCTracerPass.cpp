/* Copyright (C) 2026 zang-langyan

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>. */
#include <gcc-plugin.h>
#undef optimize
#include <plugin-version.h>
#include <tree.h>
#include <function.h>
#include <gimple.h>
#include <gimple-iterator.h>
#include <gimplify.h>
#include <tree-pass.h>
#include <context.h>
#include <stringpool.h>
#include <attribs.h>
#include <diagnostic.h>

#include <iostream>
#include "util/cctracer_config.h"

int plugin_is_GPL_compatible = 1;

static cctracer::CCTracerConfig& get_config() {
    static cctracer::CCTracerConfig config;
    static bool initialized;
    if (!initialized) {
        if (!config.load_from_ini(cctracer::getHomeDir() + "/.cctracer.ini")) {
            std::cerr << "Failed to load .cctracer.ini, using default config instead.\n";
        }
        initialized = true;
    }
    return config;
}

static bool should_instrument(function *fun) {
    tree decl = fun->decl;
    if (!decl)
        return false;

    if (fndecl_built_in_p(decl))
        return false;

    const char *name = IDENTIFIER_POINTER(DECL_NAME(decl));
    if (!strcmp(name, "__cctracer_function_entry") ||
        !strcmp(name, "__cctracer_function_exit"))
        return false;

    if (lookup_attribute("no_cctracer", DECL_ATTRIBUTES(decl))){
        return false;
    }
    if (lookup_attribute("cctracer", DECL_ATTRIBUTES(decl))){
        return true;
    }

    const char *file_name = DECL_SOURCE_FILE(decl);
    cctracer::CCTracerRules& rules = get_config().rules;
    return rules.should_instrument(
        name, 
        file_name
    );
}

static unsigned int cctracer_instrument_execute(void) {
    function *fun = cfun;
    if (!should_instrument(fun))
        return 0;

    tree decl = fun->decl;
    const char *func_name = IDENTIFIER_POINTER(DECL_NAME(decl));
    const char *file_name = DECL_SOURCE_FILE(decl);
    int line = DECL_SOURCE_LINE(decl);
    int column = DECL_SOURCE_COLUMN(decl);

    tree func_name_str = build_string_literal(strlen(func_name) + 1, func_name);
    tree file_name_str = build_string_literal(strlen(file_name) + 1, file_name);

    tree line_cst   = build_int_cst(integer_type_node, line);
    tree column_cst = build_int_cst(integer_type_node, column);

    /* entry: uint64_t __cctracer_function_entry(const char*, const char*, int, int) */
    tree entry_fndecl = build_fn_decl("__cctracer_function_entry",
        build_function_type_list(long_long_unsigned_type_node,
                                 const_ptr_type_node,
                                 const_ptr_type_node,
                                 integer_type_node,
                                 integer_type_node,
                                 NULL_TREE));
    /* exit: void __cctracer_function_exit(const char*, const char*, int, int, uint64_t) */
    tree exit_fndecl = build_fn_decl("__cctracer_function_exit",
        build_function_type_list(void_type_node,
                                 const_ptr_type_node,
                                 const_ptr_type_node,
                                 integer_type_node,
                                 integer_type_node,
                                 long_long_unsigned_type_node,
                                 NULL_TREE));

    basic_block entry_bb = ENTRY_BLOCK_PTR_FOR_FN(fun)->next_bb;
    gimple_stmt_iterator gsi = gsi_start_bb(entry_bb);

    gcall *entry_call = gimple_build_call(entry_fndecl, 4,
                                          func_name_str, file_name_str,
                                          line_cst, column_cst);
    tree begin_var = create_tmp_var(long_long_unsigned_type_node, "cctrace_begin");
    gimple_call_set_lhs(entry_call, begin_var);
    gsi_insert_before(&gsi, entry_call, GSI_SAME_STMT);

    basic_block bb;
    FOR_EACH_BB_FN(bb, fun) {
        gimple_stmt_iterator gsi;
        for (gsi = gsi_start_bb(bb); !gsi_end_p(gsi); ) {
            gimple *stmt = gsi_stmt(gsi);
            if (gimple_code(stmt) == GIMPLE_RETURN) {
                gcall *exit_call = gimple_build_call(exit_fndecl, 5,
                                                     func_name_str, file_name_str,
                                                     line_cst, column_cst,
                                                     begin_var);
                gsi_insert_before(&gsi, exit_call, GSI_SAME_STMT);
                gsi_next(&gsi);
            } else {
                gsi_next(&gsi);
            }
        }
    }

    return 0;
}

const pass_data cctracer_pass_data = {
    GIMPLE_PASS,
    "cctracer",
    OPTGROUP_NONE,
    TV_NONE,
    PROP_gimple_any,
    0, 0, 0, 0
};

class cctracer_pass : public gimple_opt_pass {
public:
    cctracer_pass(gcc::context *ctx)
        : gimple_opt_pass(cctracer_pass_data, ctx) {}

    unsigned int execute(function *fun) final override {
        return cctracer_instrument_execute();
    }
};

int plugin_init(struct plugin_name_args *plugin_info,
                struct plugin_gcc_version *version) {
    if (!plugin_default_version_check(version, &gcc_version)) {
        error("cctracer plugin: incompatible GCC version");
        return 1;
    }

    struct register_pass_info pass_info;
    pass_info.pass = new cctracer_pass(g);
    pass_info.reference_pass_name = "cfg";
    pass_info.ref_pass_instance_number = 1;
    pass_info.pos_op = PASS_POS_INSERT_AFTER;

    register_callback(plugin_info->base_name,
                      PLUGIN_PASS_MANAGER_SETUP,
                      NULL,
                      &pass_info);

    return 0;
}