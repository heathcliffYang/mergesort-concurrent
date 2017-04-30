#include <stdlib.h>
#include <stdio.h>

#include "list.h"
#include "generic_printf.h"

/**
 * @brief Create a new node with data _val_ and set the next node to _net_
 * @param val Specifiy the data to assign to the new node
 * @param next Pointer to the next node
 * @return Pointer to the created new node
 */
#if defined(lockfree)
static inline
int is_marked_ref(long i)
{
    return (int)(i & 0x1L);
}

static inline
long unset_mark(long i)
{
    i &= ~0x1L;
    return i;
}

static inline
long set_mark(long i)
{
    i |= 0x1L;
    return i;
}

static inline
long get_unmarked_ref(long w)
{
    return w & ~0x1L;
}

static inline
long get_marked_ref(long w)
{
    return w | 0x1L;
}
#endif

/**
 * @brief Initialize the linked list.
 *
 * The function will allocate a block of memory for _llist\_t_ and
 * initialize its members _head_ to NULL and _size_ to 0.
 *
 * @return Pointer to the allocated _llist\_t_
 */


llist_t *list_new()
{
    // allocate list
    llist_t *the_list = malloc(sizeof(llist_t));
#if defined(lockfree)
    // now need to create the sentinel node
    the_list->head = new_node(INT_MIN, NULL);
    the_list->tail = new_node(INT_MAX, NULL);
    the_list->head->next = the_list->tail;
#elif defined(orig)
    the_list->head = NULL;
#endif
    the_list->size = 0;

    return the_list;
}

node_t *new_node(val_t val, node_t *next)
{
    // allocate node
    node_t *node = malloc(sizeof(node_t));
    node->data = val;
    node->next = next;
    return node;
}

/**
 * @brief Insert a new node with the given value val at the head of the _list_
 * @param list The target linked list
 * @param val Specify the value
 * @return The final size of the linked list
 */
#if defined(lockfree)
node_t *list_search(llist_t *set, val_t val, node_t **left_node)
{
    node_t *left_node_next, *right_node;
    left_node_next = right_node = NULL;
    while (1) {
        node_t *t = set->head;
        node_t *t_next = set->head->next;
        while (is_marked_ref(t_next) || (t->data < val)) {
            if (!is_marked_ref(t_next)) {
                (*left_node) = t;
                left_node_next = t_next;
            }
            t = get_unmarked_ref(t_next);
            if (t == set->tail)
                break;
            t_next = t->next;
        }
        right_node = t;

        if (left_node_next == right_node) {
            if (!is_marked_ref(right_node->next))
                return right_node;
        } else {
            if (CAS_PTR(&((*left_node)->next), left_node_next, right_node) ==
                    left_node_next) {
                if (!is_marked_ref(right_node->next))
                    return right_node;
            }
        }
    }
}
#endif

int list_add(llist_t *the_list, val_t val)
{
#if defined(lockfree)
    node_t *right, *left;
    right = left = NULL;
    node_t *new_elem = new_node(val, NULL);
    while (1) {
        right = list_search(the_list, val, &left);
        new_elem->next = right;
        if (CAS_PTR(&(left->next), right, new_elem) == right) {
            FAI_U32(&(the_list->size));
            return 1;
        }
    }
#elif defined(orig)
    node_t *e = new_node(val, NULL);
    e->next = the_list->head;
    the_list->head = e;
    the_list->size++;
    return the_list->size;
#endif
}


/**
 * @brief Get the node specified by index
 * If the index is out of range, it will return NULL.
 *
 * @param list The target linked list
 * @param index Specify the index of the node in the _list_
 * @return The node at index _index_.
 */
node_t *list_get(llist_t * const list, const uint32_t index)
{
    uint32_t idx = index;
    if (!(idx < list->size))
        return NULL;
    node_t *head = list->head;
    while (idx--)
        head = head->next;
    return head;
}

/**
 * @brief Display the data of all nodes in the linked list
 * @param list The target linked list
 */
void list_print(const llist_t * const list)
{
    const node_t *cur = list->head;
    int count = (int)list->size;
    while (cur && count--) {
        xprintln(cur->data);
        cur = cur->next;
    }
}

/**
 * @brief Release the memory allocated to nodes in the linked list
 * @param list The target linked list
 */
void list_free_nodes(llist_t *list)
{
    node_t *cur = list->head;
    while (cur) {
        cur = cur->next;
        free(cur);
    }
    list->head = NULL;
}

/*
 * list_remove deletes a node with the given value val (if the value is present)
 * or does nothing (if the value is already present).
 * The deletion is logical and consists of setting the node mark bit to 1.
 */
#if defined(lockfree)
int list_remove(llist_t *the_list, val_t val)
{
    node_t *right, *left, *right_succ;
    right = left = right_succ = NULL;
    while (1) {
        right = list_search(the_list, val, &left);
        // check if we found our node
        if (right == the_list->tail || right->data != val) {
            return 0;
        }
        right_succ = right->next;
        if (!is_marked_ref(right_succ)) {
            if (CAS_PTR(&(right->next), right_succ,
                        get_marked_ref(right_succ)) == right_succ) {
                FAD_U32(&(the_list->size));
                return 1;
            }
        }
    }
    // we just logically delete it, someone else will invoke search and delete
    // it
}
#endif
