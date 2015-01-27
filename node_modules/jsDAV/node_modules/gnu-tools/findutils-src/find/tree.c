/* tree.c -- helper functions to build and evaluate the expression tree.
   Copyright (C) 1990, 91, 92, 93, 94, 2000, 2003, 2004, 2005, 2006, 2007 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>
#include "defs.h"

#include <assert.h>
#include <stdlib.h>

#include "xalloc.h"
#include "error.h"


#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define _(Text) Text
#endif
#ifdef gettext_noop
# define N_(String) gettext_noop (String)
#else
/* See locate.c for explanation as to why not use (String) */
# define N_(String) String
#endif



/* All predicates for each path to process. */
static struct predicate *predicates = NULL;

/* The root of the evaluation tree. */
static struct predicate *eval_tree  = NULL;

/* The last predicate allocated. */
static struct predicate *last_pred = NULL;


static struct predicate *scan_rest PARAMS((struct predicate **input,
				       struct predicate *head,
				       short int prev_prec));
static void merge_pred PARAMS((struct predicate *beg_list, struct predicate *end_list, struct predicate **last_p));
static struct predicate *set_new_parent PARAMS((struct predicate *curr, enum predicate_precedence high_prec, struct predicate **prevp));
static const char *cost_name PARAMS((enum EvaluationCost cost));


/* Return a pointer to a tree that represents the
   expression prior to non-unary operator *INPUT.
   Set *INPUT to point at the next input predicate node.

   Only accepts the following:
   
   <primary>
   expression		[operators of higher precedence]
   <uni_op><primary>
   (arbitrary expression)
   <uni_op>(arbitrary expression)
   
   In other words, you can not start out with a bi_op or close_paren.

   If the following operator (if any) is of a higher precedence than
   PREV_PREC, the expression just nabbed is part of a following
   expression, which really is the expression that should be handed to
   our caller, so get_expr recurses. */

struct predicate *
get_expr (struct predicate **input,
	  short int prev_prec,
	  const struct predicate* prev_pred)
{
  struct predicate *next = NULL;
  struct predicate *this_pred = (*input);

  if (*input == NULL)
    error (1, 0, _("invalid expression"));
  
  switch ((*input)->p_type)
    {
    case NO_TYPE:
      error (1, 0, _("invalid expression"));
      break;

    case BI_OP:
      /* e.g. "find . -a" */
      error (1, 0, _("invalid expression; you have used a binary operator '%s' with nothing before it."), this_pred->p_name);
      break;

    case CLOSE_PAREN:
      if ((UNI_OP == prev_pred->p_type
	  || BI_OP == prev_pred->p_type)
	  && !this_pred->artificial)
	{
	  /* e.g. "find \( -not \)" or "find \( -true -a \" */
	  error(1, 0, _("expected an expression between '%s' and ')'"),
		prev_pred->p_name);
	}
      else if ( (*input)->artificial )
	{
	  /* We have reached the end of the user-supplied predicates
	   * unexpectedly. 
	   */
	  /* e.g. "find . -true -a" */
	  error (1, 0, _("expected an expression after '%s'"), prev_pred->p_name);
	}
      else
	{
	  error (1, 0, _("invalid expression; you have too many ')'"));
	}
      break;

    case PRIMARY_TYPE:
      next = *input;
      *input = (*input)->pred_next;
      break;

    case UNI_OP:
      next = *input;
      *input = (*input)->pred_next;
      next->pred_right = get_expr (input, NEGATE_PREC, next);
      break;

    case OPEN_PAREN:
      if ( (NULL == (*input)->pred_next) || (*input)->pred_next->artificial )
	{
	  /* user typed something like "find . (", and so the ) we are
	   * looking at is from the artificial "( ) -print" that we
	   * add.
	   */
	  error (1, 0, _("invalid expression; expected to find a ')' but didn't see one.  Perhaps you need an extra predicate after '%s'"), this_pred->p_name);
	}
      prev_pred = (*input);
      *input = (*input)->pred_next;
      if ( (*input)->p_type == CLOSE_PAREN )
	{
	  error (1, 0, _("invalid expression; empty parentheses are not allowed."));
	}
      next = get_expr (input, NO_PREC, prev_pred);
      if ((*input == NULL)
	  || ((*input)->p_type != CLOSE_PAREN))
	error (1, 0, _("invalid expression; I was expecting to find a ')' somewhere but did not see one."));
      *input = (*input)->pred_next;	/* move over close */
      break;
      
    default:
      error (1, 0, _("oops -- invalid expression type!"));
      break;
    }

  /* We now have the first expression and are positioned to check
     out the next operator.  If NULL, all done.  Otherwise, if
     PREV_PREC < the current node precedence, we must continue;
     the expression we just nabbed is more tightly bound to the
     following expression than to the previous one. */
  if (*input == NULL)
    return (next);
  if ((int) (*input)->p_prec > (int) prev_prec)
    {
      next = scan_rest (input, next, prev_prec);
      if (next == NULL)
	error (1, 0, _("invalid expression"));
    }
  return (next);
}

/* Scan across the remainder of a predicate input list starting
   at *INPUT, building the rest of the expression tree to return.
   Stop at the first close parenthesis or the end of the input list.
   Assumes that get_expr has been called to nab the first element
   of the expression tree.
   
   *INPUT points to the current input predicate list element.
   It is updated as we move along the list to point to the
   terminating input element.
   HEAD points to the predicate element that was obtained
   by the call to get_expr.
   PREV_PREC is the precedence of the previous predicate element. */

static struct predicate *
scan_rest (struct predicate **input,
	   struct predicate *head,
	   short int prev_prec)
{
  struct predicate *tree;	/* The new tree we are building. */

  if ((*input == NULL) || ((*input)->p_type == CLOSE_PAREN))
    return (NULL);
  tree = head;
  while ((*input != NULL) && ((int) (*input)->p_prec > (int) prev_prec))
    {
      switch ((*input)->p_type)
	{
	case NO_TYPE:
	case PRIMARY_TYPE:
	case UNI_OP:
	case OPEN_PAREN:
	  /* I'm not sure how we get here, so it is not obvious what
	   * sort of mistakes might give rise to this condition.
	   */
	  error (1, 0, _("invalid expression"));
	  break;

	case BI_OP:
	  {
	    struct predicate *prev = (*input);
	    (*input)->pred_left = tree;
	    tree = *input;
	    *input = (*input)->pred_next;
	    tree->pred_right = get_expr (input, tree->p_prec, prev);
	    break;
	  }

	case CLOSE_PAREN:
	  return tree;

	default:
	  error (1, 0,
		 _("oops -- invalid expression type (%d)!"),
		 (int)(*input)->p_type);
	  break;
	}
    }
  return tree;
}

/* Returns true if the specified predicate is reorderable. */
static boolean
predicate_is_cost_free(const struct predicate *p)
{
  if (pred_is(p, pred_name) ||
      pred_is(p, pred_path) ||
      pred_is(p, pred_iname) ||
      pred_is(p, pred_ipath))
    {
      /* Traditionally (at least 4.1.7 through 4.2.x) GNU find always
       * optimised these cases.
       */
      return true;
    }
  else if (options.optimisation_level > 0)
    {
      if (pred_is(p, pred_and) ||
	  pred_is(p, pred_negate) ||
	  pred_is(p, pred_comma) ||
	  pred_is(p, pred_or))
	return false;
      else
	return NeedsNothing == p->p_cost;
    }
  else
    {
      return false;
    }
}

/* Prints a predicate */
void print_predicate(FILE *fp, const struct predicate *p)
{
  fprintf (fp, "%s%s%s",
	   p->p_name,
	   p->arg_text ? " " : "",
	   p->arg_text ? p->arg_text : "");
}


struct predlist 
{
  struct predicate *head;
  struct predicate *tail;
};

static void
predlist_init(struct predlist *p)
{
  p->head = p->tail = NULL;
}

static void
predlist_insert(struct predlist *list,
		struct predicate *curr,
		struct predicate **pprev)
{
  struct predicate **insertpos = &(list->head);
  
  *pprev = curr->pred_left;
  if (options.optimisation_level > 2)
    {
      /* Insert the new node in the list after any other entries which
       * are more selective.
       */
      if (0)
	while ( (*insertpos) && ((*insertpos)->est_success_rate < curr->est_success_rate) )
	  {
	    insertpos = &((*insertpos)->pred_left);
	  }
    }
  curr->pred_left = (*insertpos);
  (*insertpos) = curr;
  if (NULL == list->tail)
    list->tail = list->head;
}

static int
pred_cost_compare(const struct predicate *p1, const struct predicate *p2, boolean wantfailure)
{
  if (p1->p_cost == p2->p_cost)
    {
      if (p1->est_success_rate == p2->est_success_rate)
	return 0;
      else if (wantfailure)
	return p1->est_success_rate < p2->est_success_rate ? -1 :  1;
      else
	return p1->est_success_rate < p2->est_success_rate ?  1 : -1;
    }
  else 
    {
      return p1->p_cost < p2->p_cost ? -1 : 1;
    }
}


static void 
predlist_merge_sort(struct predlist *list,
		    struct predicate **last)
{
  struct predlist new_list;
  struct predicate *p, *q;

  if (NULL == list->head)
    return;			/* nothing to do */

  if (options.debug_options & DebugTreeOpt)
    {
      fprintf(stderr, "%s:\n", "predlist before merge sort");
      print_tree(stderr, list->head, 2);
    }
  
  calculate_derived_rates(list->head);
  predlist_init(&new_list);
  while (list->head)
    {
      /* remove head of source list */
      q = list->head;
      list->head = list->head->pred_left;
      q->pred_left = NULL;

      /* insert it into the new list */
      for (p=new_list.head; p; p=p->pred_left)
	{
	  /* If these operations are OR operations, we want to get a
	   * successful test as soon as possible, to take advantage of
	   * the short-circuit evaluation.  If they're AND, we want to
	   * get an unsuccessful result early for the same reason.
	   * Therefore we invert the sense of the comparison for the
	   * OR case.  We only want to invert the sense of the success
	   * rate comparison, not the operation cost comparison.  Hence we 
	   * pass a flag into pred_cost_compare().
	   */
	  boolean wantfailure = (OR_PREC != p->p_prec);
	  if (pred_cost_compare(p->pred_right, q->pred_right, wantfailure) >= 0)
	    break;
	}
      if (p)
	{
	  /* insert into existing list */
	  q->pred_left = p->pred_left;
	  if (NULL == q->pred_left)
	    new_list.tail = q;
	  p->pred_left = q;
	}
      else 
	{
	  q->pred_left = new_list.head;	/* prepend */
	  new_list.head = q;
	  if (NULL == new_list.tail)
	    new_list.tail = q; /* first item in new list */
	}
    }
  if (options.debug_options & DebugTreeOpt)
    {
      fprintf(stderr, "%s:\n", "predlist after merge sort");
      print_tree(stderr, new_list.head, 2);
    }
  
  calculate_derived_rates(new_list.head);
  merge_pred(new_list.head, new_list.tail, last);
  predlist_init(list);
}

static void 
merge_lists(struct predlist lists[], int nlists,
	    struct predlist *name_list,
	    struct predlist *regex_list,
	    struct predicate **last)
{
  int i;
  static void (*mergefn)(struct predlist *, struct predicate**);

  mergefn = predlist_merge_sort;
  
  mergefn(name_list,   last);
  mergefn(regex_list,  last);
  
  for (i=0; i<nlists; i++)
    mergefn(&lists[i], last);
}



static boolean 
subtree_has_side_effects(const struct predicate *p)
{
  if (p)
    {
      return p->side_effects
	|| subtree_has_side_effects(p->pred_left)
	|| subtree_has_side_effects(p->pred_right);
    }
  else
    {

      return false;
    }
}

static int
worst_cost (const struct predicate *p)
{
  if (p)
    {
      unsigned int cost_r, cost_l, worst;
      cost_l = worst_cost(p->pred_left);
      cost_r = worst_cost(p->pred_right);
      worst = (cost_l > cost_r) ? cost_l : cost_r;
      if (worst < p->p_cost)
	worst = p->p_cost;
      return worst;
    }
  else
    {
      return 0;
    }
}



static void
perform_arm_swap(struct predicate *p)
{
  struct predicate *tmp = p->pred_left->pred_right;
  p->pred_left->pred_right = p->pred_right;
  p->pred_right = tmp;
}

/* Consider swapping p->pred_left->pred_right with p->pred_right, 
 * if that yields a faster evaluation.   Normally the left predicate is 
 * evaluated first.
 *
 * If the operation is an OR, we want the left predicate to be the one that 
 * succeeds most often.   If it is an AND, we want it to be the predicate that 
 * fails most often.
 *
 * We don't consider swapping arms of an operator where their cost is
 * different or where they have side effects.
 *
 * A viable test case for this is 
 * ./find -D opt   -O3  .   \! -type f -o -type d
 * Here, the ! -type f should be evaluated first,
 * as we assume that 95% of inodes are vanilla files.
 */
static boolean
consider_arm_swap(struct predicate *p)
{
  int left_cost, right_cost;
  const char *reason = NULL;
  struct predicate **pl, **pr;

  if (BI_OP != p->p_type)
    reason = "Not a binary operation";

  if (!reason)
    {
      if (NULL == p->pred_left || NULL == p->pred_right)
	reason = "Doesn't have two arms";
    }

  
  if (!reason)
    {
      if (NULL == p->pred_left->pred_right)
	reason = "Left arm has no child on RHS";
    }
  pr = &p->pred_right;
  pl = &p->pred_left->pred_right;
  
  if (!reason)
    {
      if (subtree_has_side_effects(*pl))
	reason = "Left subtree has side-effects";
    }
  if (!reason)
    {
      if (subtree_has_side_effects(*pr))
	reason = "Right subtree has side-effects";
    }

  if (!reason)
    {
      left_cost = worst_cost(*pl);
      right_cost = worst_cost(*pr);
      
      if (left_cost < right_cost)
	{
	  reason = "efficient as-is";
	}
    }
  if (!reason)
    {
      boolean want_swap;
      
      if (left_cost == right_cost)
	{
	  /* it's a candidate */
	  float succ_rate_l = (*pl)->est_success_rate;
	  float succ_rate_r = (*pr)->est_success_rate;

	  if (options.debug_options & DebugTreeOpt)
	    {
	      fprintf(stderr, "Success rates: l=%f, r=%f\n", succ_rate_l, succ_rate_r);
	    }
	  
	  if (pred_is(p, pred_or))
	    {
	      want_swap = succ_rate_r < succ_rate_l;
	      if (!want_swap)
		reason = "Operation is OR and right success rate >= left";
	    }
	  else if (pred_is(p, pred_and))
	    {
	      want_swap = succ_rate_r > succ_rate_l;
	      if (!want_swap)
		reason = "Operation is AND and right success rate <= left";
	    }
	  else
	    {
	      want_swap = false;
	      reason = "Not AND or OR";
	    }
	}
      else 
	{
	  want_swap = true;
	}
      
      if (want_swap)
	{
	  if (options.debug_options & DebugTreeOpt)
	    {
	      fprintf(stderr, "Performing arm swap on:\n");
	      print_tree (stderr, p, 0);
	    }
	  perform_arm_swap(p);
	  return true;
	}
    }
      

  if (options.debug_options & DebugTreeOpt)
    {
      fprintf(stderr, "Not an arm swap candidate (%s):\n", reason);
      print_tree (stderr, p, 0);
    }
  return false;
}

static boolean
do_arm_swaps(struct predicate *p)
{
  if (p)
    {
      boolean swapped;
      do 
	{
	  swapped = false;
	  if (consider_arm_swap(p)
	      || do_arm_swaps(p->pred_left)
	      || do_arm_swaps(p->pred_right))
	    {
	      swapped = true;
	    }
	} while (swapped);
      return swapped;
    }
  else
    {
      return false;
    }
}



/* Optimize the ordering of the predicates in the tree.  Rearrange
   them to minimize work.  Strategies:
   * Evaluate predicates that don't need inode information first;
     the predicates are divided into 1 or more groups separated by
     predicates (if any) which have "side effects", such as printing.
     The grouping implements the partial ordering on predicates which
     those with side effects impose.

   * Place -name, -iname, -path, -ipath, -regex and -iregex at the front
     of a group, with -name, -iname, -path and -ipath ahead of
     -regex and -iregex.  Predicates which are moved to the front
     of a group by definition do not have side effects.  Both
     -regex and -iregex both use pred_regex.

     If higher optimisation levels have been selected, reordering also
     occurs according to the p_cost member of each predicate (which
     reflects the performance cost of the test).  The ordering also
     bears in mind whether these operations are more likely to succeed
     or fail.  When evauating a chain of OR conditions, we prefer
     tests likely to succeed at the front of the list.  For AND, we
     prefer tests likely to fail at the front of the list.
     
     This routine "normalizes" the predicate tree by ensuring that
     all expression predicates have AND (or OR or COMMA) parent nodes
     which are linked along the left edge of the expression tree.
     This makes manipulation of subtrees easier.  

     EVAL_TREEP points to the root pointer of the predicate tree
     to be rearranged.  opt_expr may return a new root pointer there.
     Return true if the tree contains side effects, false if not. */

static boolean
opt_expr (struct predicate **eval_treep)
{
  struct predlist regex_list={NULL,NULL}, name_list={NULL,NULL};
  struct predlist cbo_list[NumEvaluationCosts];
  int i;
  struct predicate *curr;
  struct predicate **prevp;	/* Address of `curr' node. */
  struct predicate **last_sidep; /* Last predicate with side effects. */
  PRED_FUNC pred_func;
  enum predicate_type p_type;
  boolean has_side_effects = false; /* Return value. */
  enum predicate_precedence prev_prec, /* precedence of last BI_OP in branch */
			    biop_prec; /* topmost BI_OP precedence in branch */

  if (eval_treep == NULL || *eval_treep == NULL)
    return (false);

  for (i=0; i<NumEvaluationCosts; i++)
    predlist_init(&cbo_list[i]);
  
  /* Set up to normalize tree as a left-linked list of ANDs or ORs.
     Set `curr' to the leftmost node, `prevp' to its address, and
     `pred_func' to the predicate type of its parent. */
  prevp = eval_treep;
  prev_prec = AND_PREC;
  curr = *prevp;
  while (curr->pred_left != NULL)
    {
      prevp = &curr->pred_left;
      prev_prec = curr->p_prec;	/* must be a BI_OP */
      curr = curr->pred_left;
    }

  /* Link in the appropriate BI_OP for the last expression, if needed. */
  if (curr->p_type != BI_OP)
    set_new_parent (curr, prev_prec, prevp);
  
  if (options.debug_options & (DebugExpressionTree|DebugTreeOpt))
    {
      /* Normalized tree. */
      fprintf (stderr, "Normalized Eval Tree:\n");
      print_tree (stderr, *eval_treep, 0);
    }
  
  /* Rearrange the predicates. */
  prevp = eval_treep;
  biop_prec = NO_PREC; /* not COMMA_PREC */
  if ((*prevp) && (*prevp)->p_type == BI_OP)
    biop_prec = (*prevp)->p_prec;
  while ((curr = *prevp) != NULL)
    {
      /* If there is a BI_OP of different precedence from the first
	 in the pred_left chain, create a new parent of the
	 original precedence, link the new parent to the left of the
	 previous and link CURR to the right of the new parent. 
	 This preserves the precedence of expressions in the tree
	 in case we rearrange them. */
      if (curr->p_type == BI_OP)
	{
          if (curr->p_prec != biop_prec)
	    curr = set_new_parent(curr, biop_prec, prevp);
	}
	  
      /* See which predicate type we have. */
      p_type = curr->pred_right->p_type;
      pred_func = curr->pred_right->pred_func;


      switch (p_type)
	{
	case NO_TYPE:
	case PRIMARY_TYPE:
	  /* Don't rearrange the arguments of the comma operator, it is
	     not commutative.  */
	  if (biop_prec == COMMA_PREC)
	    break;

	  /* If this predicate has no side effects, consider reordering it. */
	  if (!curr->pred_right->side_effects)		  
	    {
	      boolean reorder;
	      
	      /* If it's one of our special primaries, move it to the
		 front of the list for that primary. */
	      if (predicate_is_cost_free(curr->pred_right))
		{
		  if (options.debug_options & DebugTreeOpt)
		    {
		      fprintf(stderr, "-O%d: promoting cheap predicate ",
			      (int)options.optimisation_level);
		      print_predicate(stderr, curr->pred_right);
		      fprintf(stderr, " into name_list\n");
		    }
		  predlist_insert(&name_list, curr, prevp);
		  continue;
		}
	      
	      if (pred_func == pred_regex)
		{
		  predlist_insert(&regex_list, curr, prevp);
		  continue;
		}

	      reorder = ((options.optimisation_level > 1)
			  && (NeedsType == curr->pred_right->p_cost)
			  && !curr->pred_right->need_stat) ||
		(options.optimisation_level > 2);
	      
	      if (reorder)
		{
		  if (options.debug_options & DebugTreeOpt)
		    {
		      fprintf(stderr, "-O%d: categorising predicate ",
			      (int)options.optimisation_level);
		      print_predicate(stderr, curr->pred_right);
		      fprintf(stderr, " by cost (%s)\n",
			      cost_name(curr->pred_right->p_cost));
		    }
		  predlist_insert(&cbo_list[curr->pred_right->p_cost], curr, prevp);
		  continue;
		}
	    }
	  
	  break;

	case UNI_OP:
	  /* For NOT, check the expression trees below the NOT. */
	  curr->pred_right->side_effects
	    = opt_expr (&curr->pred_right->pred_right);
	  break;

	case BI_OP:
	  /* For nested AND or OR, recurse (AND/OR form layers on the left of
	     the tree), and continue scanning this level of AND or OR. */
	  curr->pred_right->side_effects = opt_expr (&curr->pred_right);
	  break;

	  /* At this point, get_expr and scan_rest have already removed
	     all of the user's parentheses. */

	default:
	  error (1, 0, _("oops -- invalid expression type!"));
	  break;
	}

      if (curr->pred_right->side_effects == true)
	{
	  last_sidep = prevp;

	  /* Incorporate lists and reset list pointers for this group.  */
	  merge_lists(cbo_list, NumEvaluationCosts, &name_list, &regex_list, last_sidep);
	  has_side_effects = true;
	}

      prevp = &curr->pred_left;
    }

  /* Do final list merges. */
  last_sidep = prevp;
  merge_lists(cbo_list, NumEvaluationCosts, &name_list, &regex_list, last_sidep);
  return has_side_effects;
}

static float
constrain_rate(float rate)
{
  if (rate > 1.0f)
    return 1.0;
  else if (rate < 0.0)
    return 0.0;
  else
    return rate;
}

/* Link in a new parent BI_OP node for CURR, at *PREVP, with precedence
   HIGH_PREC. */

static struct predicate *
set_new_parent (struct predicate *curr, enum predicate_precedence high_prec, struct predicate **prevp)
{
  struct predicate *new_parent;

  new_parent = xmalloc (sizeof (struct predicate));
  new_parent->p_type = BI_OP;
  new_parent->p_prec = high_prec;
  new_parent->need_stat = false;
  new_parent->need_type = false;
  new_parent->p_cost = NeedsNothing;
  
  switch (high_prec)
    {
    case COMMA_PREC:
      new_parent->pred_func = pred_comma;
      new_parent->p_name = ",";
      new_parent->est_success_rate = 1.0;
      break;
    case OR_PREC:
      new_parent->pred_func = pred_or;
      new_parent->p_name = "-o";
      new_parent->est_success_rate = constrain_rate(curr->est_success_rate);
      break;
    case AND_PREC:
      new_parent->pred_func = pred_and;
      new_parent->p_name = "-a";
      new_parent->est_success_rate = constrain_rate(curr->est_success_rate);
      break;
    default:
      ;				/* empty */
    }
  
  new_parent->side_effects = false;
  new_parent->no_default_print = false;
  new_parent->args.str = NULL;
  new_parent->pred_next = NULL;

  /* Link in new_parent.
     Pushes rest of left branch down 1 level to new_parent->pred_right. */
  new_parent->pred_left = NULL;
  new_parent->pred_right = curr;
  *prevp = new_parent;

  return new_parent;
}

/* Merge the predicate list that starts at BEG_LIST and ends at END_LIST
   into the tree at LAST_P. */

static void
merge_pred (struct predicate *beg_list, struct predicate *end_list, struct predicate **last_p)
{
  end_list->pred_left = *last_p;
  *last_p = beg_list;
}

/* Find the first node in expression tree TREE that requires
   a stat call and mark the operator above it as needing a stat
   before calling the node.   Since the expression precedences 
   are represented in the tree, some preds that need stat may not
   get executed (because the expression value is determined earlier.)
   So every expression needing stat must be marked as such, not just
   the earliest, to be sure to obtain the stat.  This still guarantees 
   that a stat is made as late as possible.  Return true if the top node 
   in TREE requires a stat, false if not. */


struct pred_cost_lookup
{
  PRED_FUNC             fn;
  enum EvaluationCost   cost;
};
static struct pred_cost_lookup costlookup[] = 
  {
    { pred_amin      ,  NeedsStatInfo        },
    { pred_and       ,  NeedsNothing,        },
    { pred_anewer    ,  NeedsStatInfo,       },
    { pred_atime     ,  NeedsStatInfo,       },
    { pred_closeparen,  NeedsNothing         },
    { pred_cmin      ,  NeedsStatInfo,       },
    { pred_cnewer    ,  NeedsStatInfo,       },
    { pred_comma     ,  NeedsNothing,        },
    { pred_ctime     ,  NeedsStatInfo,       },
    { pred_delete    ,  NeedsSyncDiskHit     },
    { pred_empty     ,  NeedsStatInfo        },
    { pred_exec      ,  NeedsEventualExec    },
    { pred_execdir   ,  NeedsEventualExec    },
    { pred_executable,  NeedsAccessInfo      },
    { pred_false     ,  NeedsNothing         }, 
    { pred_fprint    ,  NeedsNothing         }, 
    { pred_fprint0   ,  NeedsNothing         }, 
    { pred_fprintf   ,  NeedsNothing         }, 
    { pred_fstype    ,  NeedsStatInfo        }, /* true for amortised cost */
    { pred_gid       ,  NeedsStatInfo        },
    { pred_group     ,  NeedsStatInfo        },
    { pred_ilname    ,  NeedsLinkName        },
    { pred_iname     ,  NeedsNothing         },
    { pred_inum      ,  NeedsStatInfo        },
    { pred_ipath     ,  NeedsNothing         },
    { pred_links     ,  NeedsStatInfo        },
    { pred_lname     ,  NeedsLinkName        },
    { pred_ls        ,  NeedsStatInfo        },
    { pred_fls       ,  NeedsStatInfo        },
    { pred_mmin	     ,  NeedsStatInfo        },
    { pred_mtime     ,  NeedsStatInfo        },
    { pred_name	     ,  NeedsNothing         },
    { pred_negate    ,  NeedsNothing,        },
    { pred_newer     ,  NeedsStatInfo,       },
    { pred_newerXY   ,  NeedsStatInfo,       },
    { pred_nogroup   ,  NeedsStatInfo        }, /* true for amortised cost if caching is on */
    { pred_nouser    ,  NeedsStatInfo        }, /* true for amortised cost if caching is on */
    { pred_ok        ,  NeedsUserInteraction },
    { pred_okdir     ,  NeedsUserInteraction },
    { pred_openparen ,  NeedsNothing         },
    { pred_or        ,  NeedsNothing,        },
    { pred_path	     ,  NeedsNothing         },
    { pred_perm	     ,  NeedsStatInfo        },
    { pred_print     ,  NeedsNothing         },
    { pred_print0    ,  NeedsNothing         }, 
    { pred_prune     ,  NeedsNothing         },
    { pred_quit	     ,  NeedsNothing         },
    { pred_readable  ,  NeedsAccessInfo      },
    { pred_regex     ,  NeedsNothing         },
    { pred_samefile  ,  NeedsStatInfo        },
    { pred_size      ,  NeedsStatInfo        },
    { pred_true	     ,  NeedsNothing         },
    { pred_type      ,  NeedsType            },
    { pred_uid       ,  NeedsStatInfo        },
    { pred_used      ,  NeedsStatInfo        },
    { pred_user      ,  NeedsStatInfo        },
    { pred_writable  ,  NeedsAccessInfo      },
    { pred_xtype     ,  NeedsType            } /* roughly correct unless most files are symlinks */
  };
static int pred_table_sorted = 0;

static boolean
check_sorted(void *base, size_t members, size_t membersize,
	     int (*cmpfn)(const void*, const void*))
{
  const char *p = base;
  size_t i;
  for (i=1u; i<members; ++i)
    {
      int result = cmpfn(p+i*membersize, p+(i-1)*membersize);
      if (result < 0)
	return false;
      result = cmpfn(p+(i-1)*membersize, p+i*membersize);
      assert (result <= 0);
    }
  return true;
}


static int
cost_table_comparison(const void *p1, const void *p2)
{
  /* We have to compare the function pointers with memcmp(), 
   * because ISO C does not allow magnitude comparison of 
   * function pointers (just equality testing).
   */
  const struct pred_cost_lookup *pc1 = p1;
  const struct pred_cost_lookup *pc2 = p2;
  union {
    PRED_FUNC pfn;
    char mem[sizeof (PRED_FUNC)];
  } u1, u2;

  u1.pfn = pc1->fn;
  u2.pfn = pc2->fn;
  return memcmp(u1.mem, u2.mem, sizeof(u1.pfn));
}

static enum EvaluationCost
get_pred_cost(const struct predicate *p)
{
  enum EvaluationCost data_requirement_cost = NeedsNothing;
  enum EvaluationCost inherent_cost = NeedsUnknown;

  if (p->need_stat)
    {
      data_requirement_cost = NeedsStatInfo;
    }
  else if (p->need_type)
    {
      data_requirement_cost = NeedsType;
    }
  else 
    {
      data_requirement_cost = NeedsNothing;
    }
  
  if (pred_is(p, pred_exec) || pred_is(p, pred_execdir))
    {
      if (p->args.exec_vec.multiple)
	inherent_cost = NeedsEventualExec;
      else
	inherent_cost = NeedsImmediateExec;
    }
  else if (pred_is(p, pred_fprintf))
    {
      /* the parser calculated the cost for us. */
      inherent_cost = p->p_cost;
    }
  else 
    {
      struct pred_cost_lookup key;
      void *entry;

      if (!pred_table_sorted)
	{
	  qsort(costlookup,
		sizeof(costlookup)/sizeof(costlookup[0]),
		sizeof(costlookup[0]),
		cost_table_comparison);

	  if (!check_sorted(costlookup,
			    sizeof(costlookup)/sizeof(costlookup[0]),
			    sizeof(costlookup[0]),
			    cost_table_comparison))
	    {
	      error(1, 0, "Failed to sort the costlookup array (indirect).");
	    }
	  pred_table_sorted = 1;
	}
      key.fn = p->pred_func;
      entry = bsearch(&key, costlookup, 
		      sizeof(costlookup)/sizeof(costlookup[0]),
		      sizeof(costlookup[0]),
		      cost_table_comparison);
      if (entry)
	{
	  inherent_cost = ((const struct pred_cost_lookup*)entry)->cost;
	}
      else
	{
	  error(0, 0, "warning: no cost entry for predicate %s", p->p_name);
	  inherent_cost = NeedsUnknown;
	}
    }

  if (inherent_cost > data_requirement_cost)
    return inherent_cost;
  else
    return data_requirement_cost;
}

static void
estimate_costs (struct predicate *tree)
{
  if (tree)
    {
      estimate_costs(tree->pred_right);
      estimate_costs(tree->pred_left);
      
      tree->p_cost = get_pred_cost(tree);
    }
}

struct predicate*
get_eval_tree(void)
{
  return eval_tree;
}

static float 
getrate(const struct predicate *p)
{
  if (p)
    return p->est_success_rate;
  else
    return 1.0f;
}


float 
calculate_derived_rates(struct predicate *p)
{
  assert (NULL != p);

  if (p->pred_right)
    calculate_derived_rates(p->pred_right);
  if (p->pred_left)
    calculate_derived_rates(p->pred_left);

  assert (p->p_type != CLOSE_PAREN);
  assert (p->p_type != OPEN_PAREN);

  switch (p->p_type)
    {
    case NO_TYPE:
      assert (NULL == p->pred_right);
      assert (NULL == p->pred_left);
      return p->est_success_rate;
      
    case PRIMARY_TYPE:
      assert (NULL == p->pred_right);
      assert (NULL == p->pred_left);
      return p->est_success_rate;

    case UNI_OP:
      /* Unary operators must have exactly one operand */
      assert (pred_is(p, pred_negate));
      assert (NULL == p->pred_left);
      p->est_success_rate = (1.0 - p->pred_right->est_success_rate);
      return p->est_success_rate;

    case BI_OP:
      {
	float rate;
	/* Binary operators must have two operands */
	if (pred_is(p, pred_and))
	  {
	    rate = getrate(p->pred_right) * getrate(p->pred_left);
	  }
	else if (pred_is(p, pred_comma))
	  {
	    rate = 1.0f;
	  }
	else if (pred_is(p, pred_or))
	  {
	    rate = getrate(p->pred_right) + getrate(p->pred_left);
	  }
	else
	  {
	    /* only and, or and comma are BI_OP. */
	    assert (0);
	    abort ();
	  }
	p->est_success_rate = constrain_rate(rate);
      }
      return p->est_success_rate;

    case OPEN_PAREN:
    case CLOSE_PAREN:
      p->est_success_rate = 1.0;
      return p->est_success_rate;
    }
  assert (0);
  abort ();
}

/* opt_expr() rearranges predicates such that each left subtree is
 * rooted at a logical predicate (e.g. and or or).  check_normalization()
 * asserts that this property still holds.
 * 
 */
static void check_normalization(struct predicate *p, boolean at_root)
{
  if (at_root)
    {
      assert (BI_OP == p->p_type);
    }

  if (p->pred_left)
    {
      assert (BI_OP == p->pred_left->p_type);
      check_normalization(p->pred_left, false);
    }
  if (p->pred_right)
    {
      check_normalization(p->pred_right, false);
    }
}

struct predicate*
build_expression_tree(int argc, char *argv[], int end_of_leading_options)
{
  const struct parser_table *parse_entry; /* Pointer to the parsing table entry for this expression. */
  char *predicate_name;		/* Name of predicate being parsed. */
  struct predicate *cur_pred;
  const struct parser_table *entry_close, *entry_print, *entry_open;
  int i, oldi;

  predicates = NULL;
  
  /* Find where in ARGV the predicates begin by skipping the list of
   * start points.
   */
  for (i = end_of_leading_options; i < argc && !looks_like_expression(argv[i], true); i++)
    {
      /* Do nothing. */ ;
    }
  
  /* Enclose the expression in `( ... )' so a default -print will
     apply to the whole expression. */
  entry_open  = find_parser("(");
  entry_close = find_parser(")");
  entry_print = find_parser("print");
  assert (entry_open  != NULL);
  assert (entry_close != NULL);
  assert (entry_print != NULL);
  
  parse_openparen (entry_open, argv, &argc);
  last_pred->p_name = "(";
  predicates->artificial = true;
  parse_begin_user_args(argv, argc, last_pred, predicates);
  pred_sanity_check(last_pred);
  
  /* Build the input order list. */
  while (i < argc )
    {
      if (!looks_like_expression(argv[i], false))
	{
	  error (0, 0, _("paths must precede expression: %s"), argv[i]);
	  usage(stderr, 1, NULL);
	}

      predicate_name = argv[i];
      parse_entry = find_parser (predicate_name);
      if (parse_entry == NULL)
	{
	  /* Command line option not recognized */
	  error (1, 0, _("unknown predicate `%s'"), predicate_name);
	}

      /* We have recognised a test of the form -foo.  Eat that, 
       * unless it is a predicate like -newerXY.
       */
      if (parse_entry->type != ARG_SPECIAL_PARSE)
	{
	  i++;
	}
      oldi = i;
      if (!(*(parse_entry->parser_func)) (parse_entry, argv, &i))
	{
	  if (argv[i])
	    {
	      if ( (ARG_SPECIAL_PARSE == parse_entry->type) && (i == oldi) )
		{
		  /* The special parse function spat out the
		   * predicate.  It must be invalid, or not tasty.
		   */
		  error (1, 0, _("invalid predicate `%s'"),
			 predicate_name);
		}
	      else
		{
		  error (1, 0, _("invalid argument `%s' to `%s'"),
			 argv[i], predicate_name);
		}
	    }
	  else
	    {
	      /* Command line option requires an argument */
	      error (1, 0, _("missing argument to `%s'"), predicate_name);
	    }
	}
      else
	{
	  last_pred->p_name = predicate_name;
	  
	  /* If the parser consumed an argument, save it. */
	  if (i != oldi)
	    last_pred->arg_text = argv[oldi];
	  else
	    last_pred->arg_text = NULL;
	}
      pred_sanity_check(last_pred);
      pred_sanity_check(predicates); /* XXX: expensive */
    }
  parse_end_user_args(argv, argc, last_pred, predicates);
  if (predicates->pred_next == NULL)
    {
      /* No predicates that do something other than set a global variable
	 were given; remove the unneeded initial `(' and add `-print'. */
      cur_pred = predicates;
      predicates = last_pred = predicates->pred_next;
      free (cur_pred);
      parse_print (entry_print, argv, &argc);
      last_pred->p_name = "-print";
      pred_sanity_check(last_pred); 
      pred_sanity_check(predicates); /* XXX: expensive */
    }
  else if (!default_prints (predicates->pred_next))
    {
      /* One or more predicates that produce output were given;
	 remove the unneeded initial `('. */
      cur_pred = predicates;
      predicates = predicates->pred_next;
      pred_sanity_check(predicates); /* XXX: expensive */
      free (cur_pred);
    }
  else
    {
      /* `( user-supplied-expression ) -print'. */
      parse_closeparen (entry_close, argv, &argc);
      last_pred->p_name = ")";
      last_pred->artificial = true;
      pred_sanity_check(last_pred);
      parse_print (entry_print, argv, &argc);
      last_pred->p_name = "-print";
      last_pred->artificial = true;
      pred_sanity_check(last_pred);
      pred_sanity_check(predicates); /* XXX: expensive */
    }

  if (options.debug_options & (DebugExpressionTree|DebugTreeOpt))
    {
      fprintf (stderr, "Predicate List:\n");
      print_list (stderr, predicates);
    }
  
  /* do a sanity check */
  check_option_combinations(predicates);
  pred_sanity_check(predicates);
  
  /* Done parsing the predicates.  Build the evaluation tree. */
  cur_pred = predicates;
  eval_tree = get_expr (&cur_pred, NO_PREC, NULL);
  calculate_derived_rates(eval_tree);
  
  /* Check if we have any left-over predicates (this fixes
   * Debian bug #185202).
   */
  if (cur_pred != NULL)
    {
      /* cur_pred->p_name is often NULL here */
      if (pred_is(cur_pred, pred_closeparen))
	{
	  /* e.g. "find \( -true \) \)" */
	  error (1, 0, _("you have too many ')'"));
	}
      else
	{
	  if (cur_pred->p_name)
	    error (1, 0, _("unexpected extra predicate '%s'"), cur_pred->p_name);
	  else
	    error (1, 0, _("unexpected extra predicate"));
	}
    }
  
  if (options.debug_options & (DebugExpressionTree|DebugTreeOpt))
    {
      fprintf (stderr, "Eval Tree:\n");
      print_tree (stderr, eval_tree, 0);
    }

  estimate_costs(eval_tree);
  
  /* Rearrange the eval tree in optimal-predicate order. */
  opt_expr (&eval_tree);

  /* Check that the tree is in normalised order (opt_expr does this) */
  check_normalization(eval_tree, true);
  
  do_arm_swaps(eval_tree);
  
  /* Check that the tree is still in normalised order */
  check_normalization(eval_tree, true);

  if (options.debug_options & (DebugExpressionTree|DebugTreeOpt))
    {
      fprintf (stderr, "Optimized Eval Tree:\n");
      print_tree (stderr, eval_tree, 0);
      fprintf (stderr, "Optimized command line:\n");
      print_optlist(stderr, eval_tree);
      fprintf(stderr, "\n");
    }

  return eval_tree;
}

/* Initialise the performance data for a predicate. 
 */
static void
init_pred_perf(struct predicate *pred)
{
  struct predicate_performance_info *p = &pred->perf;
  p->visits = p->successes = 0;
}


/* Return a pointer to a new predicate structure, which has been
   linked in as the last one in the predicates list.

   Set `predicates' to point to the start of the predicates list.
   Set `last_pred' to point to the new last predicate in the list.
   
   Set all cells in the new structure to the default values. */

struct predicate *
get_new_pred (const struct parser_table *entry)
{
  register struct predicate *new_pred;
  (void) entry;

  /* Options should not be turned into predicates. */
  assert (entry->type != ARG_OPTION);
  assert (entry->type != ARG_POSITIONAL_OPTION);
  
  if (predicates == NULL)
    {
      predicates = (struct predicate *)
	xmalloc (sizeof (struct predicate));
      last_pred = predicates;
    }
  else
    {
      new_pred = xmalloc (sizeof (struct predicate));
      last_pred->pred_next = new_pred;
      last_pred = new_pred;
    }
  last_pred->parser_entry = entry;
  last_pred->pred_func = NULL;
  last_pred->p_name = NULL;
  last_pred->p_type = NO_TYPE;
  last_pred->p_prec = NO_PREC;
  last_pred->side_effects = false;
  last_pred->no_default_print = false;
  last_pred->need_stat = true;
  last_pred->need_type = true;
  last_pred->args.str = NULL;
  last_pred->pred_next = NULL;
  last_pred->pred_left = NULL;
  last_pred->pred_right = NULL;
  last_pred->literal_control_chars = options.literal_control_chars;
  last_pred->artificial = false;
  last_pred->est_success_rate = 1.0;
  init_pred_perf(last_pred);
  return last_pred;
}

/* Return a pointer to a new predicate, with operator check.
   Like get_new_pred, but it checks to make sure that the previous
   predicate is an operator.  If it isn't, the AND operator is inserted. */

struct predicate *
get_new_pred_chk_op (const struct parser_table *entry)
{
  struct predicate *new_pred;
  static const struct parser_table *entry_and = NULL;

  /* Locate the entry in the parser table for the "and" operator */
  if (NULL == entry_and)
    entry_and = find_parser("and");

  /* Check that it's actually there. If not, that is a bug.*/
  assert (entry_and != NULL);	

  if (last_pred)
    switch (last_pred->p_type)
      {
      case NO_TYPE:
	error (1, 0, _("oops -- invalid default insertion of and!"));
	break;

      case PRIMARY_TYPE:
      case CLOSE_PAREN:
	/* We need to interpose the and operator. */
	new_pred = get_new_pred (entry_and);
	new_pred->pred_func = pred_and;
	new_pred->p_name = "-a";
	new_pred->p_type = BI_OP;
	new_pred->p_prec = AND_PREC;
	new_pred->need_stat = false;
	new_pred->need_type = false;
	new_pred->args.str = NULL;
	new_pred->side_effects = false;
	new_pred->no_default_print = false;
	break;

      default:
	break;
      }
  
  new_pred = get_new_pred (entry);
  new_pred->parser_entry = entry;
  return new_pred;
}

struct cost_assoc
{
  enum EvaluationCost cost;
  char *name;
};
struct cost_assoc cost_table[] = 
  {
    { NeedsNothing,         "Nothing" },
    { NeedsType,	    "Type" },
    { NeedsStatInfo,	    "StatInfo" },
    { NeedsLinkName,	    "LinkName" },
    { NeedsAccessInfo,	    "AccessInfo" },
    { NeedsSyncDiskHit,	    "SyncDiskHit" },
    { NeedsEventualExec,    "EventualExec" },
    { NeedsImmediateExec,   "ImmediateExec" },
    { NeedsUserInteraction, "UserInteraction" },
    { NeedsUnknown,	    "Unknown" }
  };

struct prec_assoc
{
  short prec;
  char *prec_name;
};

static struct prec_assoc prec_table[] =
{
  {NO_PREC, "no"},
  {COMMA_PREC, "comma"},
  {OR_PREC, "or"},
  {AND_PREC, "and"},
  {NEGATE_PREC, "negate"},
  {MAX_PREC, "max"},
  {-1, "unknown "}
};

struct op_assoc
{
  short type;
  char *type_name;
};

static struct op_assoc type_table[] =
{
  {NO_TYPE,      "no"},
  {PRIMARY_TYPE, "primary"},
  {UNI_OP,       "uni_op"},
  {BI_OP,        "bi_op"},
  {OPEN_PAREN,   "open_paren  "},
  {CLOSE_PAREN,  "close_paren "},
  {-1,           "unknown"}
};

static const char *
cost_name (enum EvaluationCost cost)
{
  unsigned int i;
  unsigned int n = sizeof(cost_table)/sizeof(cost_table[0]);
  
  for (i = 0; i<n; ++i)
    if (cost_table[i].cost == cost)
      return cost_table[i].name;
  return "unknown";
}


static char *
type_name (type)
     short type;
{
  int i;

  for (i = 0; type_table[i].type != (short) -1; i++)
    if (type_table[i].type == type)
      break;
  return (type_table[i].type_name);
}

static char *
prec_name (prec)
     short prec;
{
  int i;

  for (i = 0; prec_table[i].prec != (short) -1; i++)
    if (prec_table[i].prec == prec)
      break;
  return (prec_table[i].prec_name);
}


/* Walk the expression tree NODE to stdout.
   INDENT is the number of levels to indent the left margin. */

void
print_tree (FILE *fp, struct predicate *node, int indent)
{
  int i;

  if (node == NULL)
    return;
  for (i = 0; i < indent; i++)
    fprintf (fp, "    ");
  fprintf (fp, "pred=[");
  print_predicate(fp, node);
  fprintf (fp, "] type=%s prec=%s",
	  type_name (node->p_type), prec_name (node->p_prec));
  fprintf (fp, " cost=%s rate=%#03.2g %sside effects ",
	   cost_name(node->p_cost),
	   node->est_success_rate,
	   (node->side_effects ? "" : "no "));
  
  if (node->need_stat || node->need_type)
    {
      int comma = 0;
      
      fprintf (fp, "Needs ");
      if (node->need_stat)
	{
	  fprintf (fp, "stat");
	  comma = 1;
	}
      if (node->need_type)
	{
	  fprintf (fp, "%stype", comma ? "," : "");
	}
    }
  fprintf (fp, "\n");


  for (i = 0; i < indent; i++)
    fprintf (fp, "    ");
  if (NULL == node->pred_left && NULL == node->pred_right)
    {
      fprintf (fp, "no children.\n");
    }
  else
    {
      if (node->pred_left)
	{
	  fprintf (fp, "left:\n");
	  print_tree (fp, node->pred_left, indent + 1);
	}
      else 
	{
	  fprintf (fp, "no left.\n");
	}
      
      for (i = 0; i < indent; i++)
	fprintf (fp, "    ");
      if (node->pred_right)
	{
	  fprintf (fp, "right:\n");
	  print_tree (fp, node->pred_right, indent + 1);
	}
      else
	{
	  fprintf (fp, "no right.\n");
	}
    }
}
