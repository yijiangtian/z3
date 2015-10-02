/*++
Module Name:

    theory_str.h

Abstract:

    String Theory Plugin

Author:

    Murphy Berzish (mtrberzi) 2015-09-03

Revision History:

--*/
#ifndef _THEORY_STR_H_
#define _THEORY_STR_H_

#include"smt_theory.h"
#include"trail.h"
#include"th_rewriter.h"
#include"value_factory.h"
#include"smt_model_generator.h"
#include"arith_decl_plugin.h"
#include<set>
#include<stack>

namespace smt {

    class str_value_factory : public value_factory {
        str_util m_util;
    public:
        str_value_factory(ast_manager & m, family_id fid) :
            value_factory(m, fid),
            m_util(m) {}
        virtual ~str_value_factory() {}
        virtual expr * get_some_value(sort * s) {
            return m_util.mk_string("some value");
        }
        virtual bool get_some_values(sort * s, expr_ref & v1, expr_ref & v2) {
            v1 = m_util.mk_string("value 1");
            v2 = m_util.mk_string("value 2");
            return true;
        }
        virtual expr * get_fresh_value(sort * s) {
            // TODO this may be causing crashes in model gen? investigate
            //return m_util.mk_fresh_string();
            NOT_IMPLEMENTED_YET();
        }
        virtual void register_value(expr * n) { /* Ignore */ }
    };

    class theory_str : public theory {
        struct T_cut
        {
            int level;
            std::map<expr*, int> vars;

            T_cut() {
              level = -100;
            }
        };
    protected:
        bool search_started;
        arith_util m_autil;
        str_util m_strutil;

        str_value_factory * m_factory;

        ptr_vector<enode> m_basicstr_axiom_todo;
        svector<std::pair<enode*,enode*> > m_str_eq_todo;
        ptr_vector<enode> m_concat_axiom_todo;

        int tmpStringVarCount;
        int tmpXorVarCount;
        std::map<std::pair<expr*, expr*>, std::map<int, expr*> > varForBreakConcat;

        bool avoidLoopCut;
        bool loopDetected;
        std::map<expr*, std::stack<T_cut *> > cut_var_map;

        std::set<expr*> variable_set;
        std::set<expr*> internal_variable_set;
    protected:
        void assert_axiom(expr * e);
        void assert_implication(expr * premise, expr * conclusion);

        app * mk_strlen(expr * e);
        expr * mk_concat(expr * n1, expr * n2);
        expr * mk_concat_const_str(expr * n1, expr * n2);

        app * mk_int(int n);

        void check_and_init_cut_var(expr * node);
        void add_cut_info_one_node(expr * baseNode, int slevel, expr * node);
        void add_cut_info_merge(expr * destNode, int slevel, expr * srcNode);
        bool has_self_cut(expr * n1, expr * n2);

        app * mk_nonempty_str_var();
        app * mk_internal_xor_var();

        bool is_concat(app const * a) const { return a->is_app_of(get_id(), OP_STRCAT); }
        bool is_concat(enode const * n) const { return is_concat(n->get_owner()); }
        bool is_string(app const * a) const { return a->is_app_of(get_id(), OP_STR); }
        bool is_string(enode const * n) const { return is_string(n->get_owner()); }
        void instantiate_concat_axiom(enode * cat);
        void instantiate_basic_string_axioms(enode * str);
        void instantiate_str_eq_length_axiom(enode * lhs, enode * rhs);

        void set_up_axioms(expr * ex);
        void handle_equality(expr * lhs, expr * rhs);

        app * mk_value_helper(app * n);
        expr * get_eqc_value(expr * n, bool & hasEqcValue);
        bool in_same_eqc(expr * n1, expr * n2);

        bool can_two_nodes_eq(expr * n1, expr * n2);
        bool can_concat_eq_str(expr * concat, std::string str);
        bool can_concat_eq_concat(expr * concat1, expr * concat2);

        void get_nodes_in_concat(expr * node, ptr_vector<expr> & nodeList);
        expr * simplify_concat(expr * node);

        void simplify_parent(expr * nn, expr * eq_str);

        void simplify_concat_equality(expr * lhs, expr * rhs);
        void solve_concat_eq_str(expr * concat, expr * str);

        bool is_concat_eq_type1(expr * concatAst1, expr * concatAst2);
        bool is_concat_eq_type2(expr * concatAst1, expr * concatAst2);
        bool is_concat_eq_type3(expr * concatAst1, expr * concatAst2);
        bool is_concat_eq_type4(expr * concatAst1, expr * concatAst2);
        bool is_concat_eq_type5(expr * concatAst1, expr * concatAst2);
        bool is_concat_eq_type6(expr * concatAst1, expr * concatAst2);

        void process_concat_eq_type1(expr * concatAst1, expr * concatAst2);
        void process_concat_eq_type2(expr * concatAst1, expr * concatAst2);
        void process_concat_eq_type3(expr * concatAst1, expr * concatAst2);
        void process_concat_eq_type4(expr * concatAst1, expr * concatAst2);
        void process_concat_eq_type5(expr * concatAst1, expr * concatAst2);
        void process_concat_eq_type6(expr * concatAst1, expr * concatAst2);

        bool new_eq_check(expr * lhs, expr * rhs);
        void group_terms_by_eqc(expr * n, std::set<expr*> & concats, std::set<expr*> & vars, std::set<expr*> & consts);

        void dump_assignments();
    public:
        theory_str(ast_manager & m);
        virtual ~theory_str();
    protected:
        virtual bool internalize_atom(app * atom, bool gate_ctx);
        virtual bool internalize_term(app * term);

        virtual void new_eq_eh(theory_var, theory_var);
        virtual void new_diseq_eh(theory_var, theory_var);

        virtual theory* mk_fresh(context*) { return alloc(theory_str, get_manager()); }
        virtual void init_search_eh();
        virtual void relevant_eh(app * n);
        virtual void assign_eh(bool_var v, bool is_true);
        virtual void push_scope_eh();
        virtual void pop_scope_eh(unsigned num_scopes);
        virtual void reset_eh();

        virtual bool can_propagate();
        virtual void propagate();

        virtual final_check_status final_check_eh();
        void attach_new_th_var(enode * n);

        virtual void init_model(model_generator & m);
        virtual model_value_proc * mk_value(enode * n, model_generator & mg);
        virtual void finalize_model(model_generator & mg);
    };

};

#endif /* _THEORY_STR_H_ */
