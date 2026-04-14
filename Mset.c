// COMP2521 24T3 - Assignment 1
// Implementation of the Multiset ADT
// Written by Christian Zantua z5544752 on October 2024

// This program is an implementation of the Multiset ADT. It allows
// users to create multisets, and insert, delete, and analyze elements
// in these multisets. The program makes use of height-balanced binary
// trees to implement the multisets for a more efficient implementation.
// However, the program is unable to make use of multiset cursors.

// Acknowledgements:
// - Implementations of bstSearch, bstDelete, and bstJoin taken from
// https://cgi.cse.unsw.edu.au/~cs2521/24T3/lectures/Week3Mon-bst.pdf
// - Implementations of avlInsert, avlBalance, avlRebalance, 
//	avlRotateRight, and avlRotateLeft taken from
// https://cgi.cse.unsw.edu.au/~cs2521/24T3/lectures/Week4Mon-avl.pdf

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "Mset.h"
#include "MsetStructs.h"

static int MsetSizeHelper(struct node *head);
static void treeFree(struct node *tree);
static int MsetCountHelper(struct node *head);
static struct node *newNode(int item, int count);
static void treePrint(bool *leftmost, struct node *tree, FILE *file);
struct node *treeDelete(struct node *tree, int item);
static struct node *treeJoin(struct node *tree1, struct node *tree2);
static Mset checkUnion(Mset newMset, struct node *tree);
static struct node *MsetUnionInsertMany(struct node *tree, int item,
										int amount);
static struct node *treeSearch(struct node *head, int item);
static Mset checkIntersection(Mset newMset, struct node *tree1,
								struct node *tree2);
static bool checkIncluded(struct node *tree1, struct node *tree2);
static bool checkCounts(struct node *head, int item, int count);
static bool checkEqual(struct node *tree1, struct node *tree2);
static bool checkEqualCounts(struct node *head, int item, int count);
static int storeInArray(struct item items[], struct node *tree, int counter);
static void arrangeArray(struct item arrayToOrder[], int count, int k);
static int treeBalance(struct node *tree);
static struct node *rotateRight(struct node *tree);
static struct node *rotateLeft(struct node *tree);
static int treeHeight(struct node *tree);
static struct node *treeRebalance(struct node *tree);
static struct node *MsetInsertHelper(struct node *tree, int item);
static struct node *MsetInsertManyHelper(struct node *tree, int item,
										int amount);
static struct node *MsetDeleteHelper(struct node *tree, int item);
static struct node *MsetDeleteManyHelper(struct node *tree, int item,
										int amount);

////////////////////////////////////////////////////////////////////////
// Basic Operations

/**
 * Creates a new empty multiset.
 */
Mset MsetNew(void) {

	struct mset *newMset = malloc(sizeof(struct mset));

	if (newMset == NULL) {
		fprintf(stderr, "error: allocation of memory failed\n");
		exit(EXIT_FAILURE);
	}

	if (newMset == NULL) {
		return NULL;
	}

	newMset->tree = NULL;
	return newMset;
}

/**
* Frees all memory allocated to the multiset.
*/
void MsetFree(Mset s) {

	if (s == NULL) {
		return;
	}
	
	treeFree(s->tree);
	free(s);
	return;
}

// This function frees the left and right subtrees of the tree,
// then it frees the root.
static void treeFree(struct node *tree) {

	if (tree == NULL) {
		return;
	}

	treeFree(tree->left);
	treeFree(tree->right);
	free(tree);
}

// This function allocates and initializes a new node for the tree.
static struct node *newNode(int item, int count) {

	struct node *newNode = malloc(sizeof(struct node));

	if (newNode == NULL) {
		fprintf(stderr, "error: allocation of memory failed\n");
		exit(EXIT_FAILURE);
	}

	newNode->elem = item;
	newNode->count = count;
	newNode->left = NULL;
	newNode->right = NULL;

	return newNode;
}

/**
* Inserts one of an item into the multiset. Does nothing if the item is
* equal to UNDEFINED.
*/
void MsetInsert(Mset s, int item) {

	if (item == UNDEFINED) {
		return;
	} else {
		s->tree = MsetInsertHelper(s->tree, item);
	}

	return;
}

// Function traverses the tree and inserts the new node
// in its proper position while recursively balancing the tree.
// The following code was adapted from https://cgi.cse.unsw.edu.au/~cs2521/24T3/lectures/Week4Mon-avl.pdf
static struct node *MsetInsertHelper(struct node *tree, int item) {

	struct node *nodeToAdd = newNode(item, 1);

	if (tree == NULL) {
		return nodeToAdd;
	} else if (tree->elem > item) {
		tree->left = MsetInsertHelper(tree->left, item);
	} else if (tree->elem < item) {
		tree->right = MsetInsertHelper(tree->right, item);
	}
	
	// If the tree already has the element, the element's count
	// increases by 1.
	else if (tree->elem == item) {
		tree->count += 1;
	}

	return treeRebalance(tree);
}

/**
* Inserts the given amount of an item into the multiset. Does nothing
* if the item is equal to UNDEFINED or the given amount is 0 or less.
*/
void MsetInsertMany(Mset s, int item, int amount) {

	if (item == UNDEFINED || amount <= 0) {
		return;
	} else {
		s->tree = MsetInsertManyHelper(s->tree, item, amount);
	}

	return;
}

// Function traverses the tree and inserts the new node
// in its proper position while recursively balancing the tree.
// The following code was adapted from https://cgi.cse.unsw.edu.au/~cs2521/24T3/lectures/Week4Mon-avl.pdf
static struct node *MsetInsertManyHelper(struct node *tree, int item,
											int amount) {

	struct node *nodeToAdd = newNode(item, amount);

	if (tree == NULL) {
		return nodeToAdd;
	} else if (tree->elem > item) {
		tree->left = MsetInsertManyHelper(tree->left, item, amount);
	} else if (tree->elem < item) {
		tree->right = MsetInsertManyHelper(tree->right, item, amount);
	}

	// If the tree already has the element, the element's count
	// increases by the given amount.
	else if (tree->elem == item) {
		tree->count += amount;
	}

	return treeRebalance(tree);
}

/**
* Deletes one of an item from the multiset.
*/
void MsetDelete(Mset s, int item) {

	s->tree = MsetDeleteHelper(s->tree, item);

	return;
}

// Function traverses the tree and finds the item to delete.
static struct node *MsetDeleteHelper(struct node *tree, int item) {

	if (tree == NULL) {
		return NULL;
	}

	struct node *curr = tree;

	while (curr != NULL) {

		if (item < curr->elem) {
			curr = curr->left;
		} else if (item > curr->elem) {
			curr = curr->right;
		}

		// If the item has a count higher than 1, meaning they do not
		// have to be removed from tree, their count is reduced by one.
		else if (item == curr->elem && curr->count > 1) {
			curr->count--;
			return tree;
		}

		// Otherwise, the node is deleted and the tree is recursively 
		// rebalanced afterwards.
		else {
			treeDelete(tree, item);
			return treeRebalance(tree);
		}
	}

	return tree;
}

// Function recursively traverses the tree and frees the
// node to delete.
// The following code was adapted from https://cgi.cse.unsw.edu.au/~cs2521/24T3/lectures/Week3Mon-bst.pdf.
struct node *treeDelete(struct node *tree, int item) {

	if (tree == NULL) {
		return NULL;
	} else if (item < tree->elem) {
		tree->left = treeDelete(tree->left, item);
	} else if (item > tree->elem) {
		tree->right = treeDelete(tree->right, item);
	} else {
		struct node *temp = tree;

		// If the left subtree is null, the deleted node
		// is replaced by its right subtree.
		if (tree->left == NULL) {
			tree = tree->right;
		} 
		
		// If the right subtree is null, the deleted node
		// is replaced by its left subtree.
		else if (tree->right == NULL) {
			tree = tree->left;
		} 
		
		// If the deleted node has two children, they
		// are joined.
		else {
			tree = treeJoin(tree->left, tree->right);
		}

		free(temp);
	}

	return tree;
}

// Function joins the left and right subtrees of the given node.
// The following code was adapted from https://cgi.cse.unsw.edu.au/~cs2521/24T3/lectures/Week3Mon-bst.pdf
static struct node *treeJoin(struct node *tree1, struct node *tree2) {

	if (tree1 == NULL) {
		return tree2;
	} else if (tree2 == NULL) {
		return tree1;
	} else {
		struct node *curr = tree2;
		struct node *parent = NULL;

		while (curr->left != NULL) {
			parent = curr;
			curr = curr->left;
		}

		if (parent != NULL) {
			parent->left = curr->right;
			curr->right = tree2;
		}

		curr->left = tree1;
		return curr;
	}
}

/**
* Deletes the given amount of an item from the multiset.
*/
void MsetDeleteMany(Mset s, int item, int amount) {

	s->tree = MsetDeleteManyHelper(s->tree, item, amount);

	return;
}

// Function traverses the tree and finds the item to delete.
static struct node *MsetDeleteManyHelper(struct node *tree, int item,
											int amount) {

	if (tree == NULL) {
		return NULL;
	}

	struct node *curr = tree;

	while (curr != NULL) {

		if (item < curr->elem) {
			curr = curr->left;
		} else if (item > curr->elem) {
			curr = curr->right;
		} 
		
		// If the item's count minus the amount to delete, is
		// greater than zero, meaning they do not have to be removed
		// from the tree, their count is reduced by the amount to delete.
		else if (item == curr->elem && (curr->count - amount) > 0) {
			curr->count -= amount;
			return tree;
		} 
		
		// Otherwise, the node is deleted and the tree is recursively 
		// rebalanced afterwards.
		else {
			treeDelete(tree, item);
			return treeRebalance(tree);
		}
	}

	return tree;
}

// Function returns the height of the given node.
static int treeHeight(struct node *tree) {

	// Empty tree has a height of -1 so return
	// -1 when tree is NULL.
	if (tree == NULL) {
		return -1;
	}

	// Recursively traverse the left and right subtrees
	// adding 1 to the height while doing so.
	int leftHeight = treeHeight(tree->left);
	int rightHeight = treeHeight(tree->right);

	// If the height of the left subtree is greater than
	// the right subtree, the height of the node is the 
	// left subtree.
	if (leftHeight > rightHeight) {
		return 1 + leftHeight;
	}

	// If the height of the right subtree is greater than
	// the right subtree, the height of the node is the
	// right subtree.
	else {
		return 1 + rightHeight;
	}
}

// Function checks whether the node needs to be balanced
// and returns the difference between left and right subtree heights.
// The following code was adapted from https://cgi.cse.unsw.edu.au/~cs2521/24T3/lectures/Week4Mon-avl.pdf.
static int treeBalance(struct node *tree) {

	// If the absolute value of the difference between
	// left and right subtree heights is greater than 1,
	// balance is necessary.
	return treeHeight(tree->left) - treeHeight(tree->right);
}

// Functions rotates the node to the right.
// The following code was adapted from https://cgi.cse.unsw.edu.au/~cs2521/24T3/lectures/Week4Mon-avl.pdf.
static struct node *rotateRight(struct node *tree) {
	
	if (tree == NULL || tree->left == NULL) {
		return tree;
	}

	struct node *newHead = tree->left;
	tree->left = newHead->right;
	newHead->right = tree;

	return newHead;
}

// Function rotates the node to the left.
// The following code was adapted from https://cgi.cse.unsw.edu.au/~cs2521/24T3/lectures/Week4Mon-avl.pdf.
static struct node *rotateLeft(struct node *tree) {
	
	if (tree == NULL || tree->right == NULL) {
		return tree;
	}

	struct node *newHead = tree->right;
	tree->right = newHead->left;
	newHead->left = tree;

	return newHead;
}

// Function rebalances the tree when necessary.
// The following code was adapted from https://cgi.cse.unsw.edu.au/~cs2521/24T3/lectures/Week4Mon-avl.pdf.
static struct node *treeRebalance(struct node *tree) {

	int bal = treeBalance(tree);

	if (bal > 1) {
		if (treeBalance(tree->left) < 0) {
			tree->left = rotateLeft(tree->left);
		}
		tree = rotateRight(tree);
	} else if (bal < -1) {
		if (treeBalance(tree->right) > 0) {
			tree->right = rotateRight(tree->right);
		}
		tree = rotateLeft(tree);
	}

	return tree;
}

/**
* Returns the number of distinct elements in the multiset.
*/
int MsetSize(Mset s) {

	int size = MsetSizeHelper(s->tree);
	return size;
}

// Function returns the size of the tree.
static int MsetSizeHelper(struct node *head) {

	if (head == NULL) {
		return 0;
	}

	// Recursively traverse the tree and add 1 to the size.
	return 1 + MsetSizeHelper(head->left) + MsetSizeHelper(head->right);
}

/**
* Returns the sum of counts of all elements in the multiset.
*/
int MsetTotalCount(Mset s) {

	int count = MsetCountHelper(s->tree);
	return count;
}

// Function returns the sum of all multiset elements.
static int MsetCountHelper(struct node *head) {

	if (head == NULL) {
		return 0;
	}

	// Recursively traverses through the tree and
	// adds the current node's count to the total count.
	return head->count + MsetCountHelper(head->left) +
			MsetCountHelper(head->right);
}

/**
* Returns the count of an item in the multiset, or 0 if it doesn't
* occur in the multiset.
*/
int MsetGetCount(Mset s, int item) {

	if (s == NULL) {
		return 0;
	}

	struct node *curr = s->tree;
	int count = 0;

	while (curr != NULL) {
		if (item < curr->elem) {
			curr = curr->left;
		} else if (item > curr->elem) {
			curr = curr->right;
		} else if (item == curr->elem) {
			count = curr->count;
			return count;
		}
	}

	return count;
}

/**
* Prints the multiset to a file.
* The elements of the multiset should be printed in ascending order
* inside a pair of curly braces, with elements separated by a comma
* and space. Each element should be printed inside a pair of
* parentheses with its count, separated by a comma and space.
*/
void MsetPrint(Mset s, FILE *file) {

	if (s == NULL) {
		fprintf(file, "{}");
		return;
	}

	bool leftmost = true;

	fprintf(file, "{");
	treePrint(&leftmost, s->tree, file);
	fprintf(file, "}");
	return;
}

// Function prints the elements of the multiset.
static void treePrint(bool *leftmost, struct node *tree, FILE *file) {

	if (tree == NULL) {
		return;
	}

	treePrint(leftmost, tree->left, file);

	// When the node is the first to be printed, it is printed
	// without the comma before it.
	if (*leftmost == true) {
		fprintf(file, "(%d, %d)", tree->elem, tree->count);
		*leftmost = false;
	} 
	
	// When the node is node the leftmost multiset element,
	// it is printed with a comma before it.
	else {
		fprintf(file, ", (%d, %d)", tree->elem, tree->count);
	}

	treePrint(leftmost, tree->right, file);
}

////////////////////////////////////////////////////////////////////////
// Advanced Operations

/**
* Returns a new multiset representing the union of the two given
* multisets.
*/
Mset MsetUnion(Mset s1, Mset s2) {

	Mset msetUnion = MsetNew();
	checkUnion(msetUnion, s1->tree);
	checkUnion(msetUnion, s2->tree);

	return msetUnion;
}

// Function traverses through each mset and inserts each item into
// the new mset. Returns the new mset.
static Mset checkUnion(Mset newMset, struct node *tree) {

	if (tree == NULL) {
		return NULL;
	}

	newMset->tree = MsetUnionInsertMany(newMset->tree, tree->elem,
											tree->count);

	checkUnion(newMset, tree->left);
	checkUnion(newMset, tree->right);

	return newMset;
}

// Function traverses the tree and inserts the new node
// in its proper position while recursively balancing the tree.
// The following code was adapted from https://cgi.cse.unsw.edu.au/~cs2521/24T3/lectures/Week4Mon-avl.pdf
static struct node *MsetUnionInsertMany(struct node *tree, int item,
											int amount) {

	if (item == UNDEFINED || amount <= 0) {
		return NULL;
	}

	else {

		struct node *nodeToAdd = newNode(item, amount);

		if (tree == NULL) {
			return nodeToAdd;
		} else if (tree->elem > item) {
			tree->left = MsetUnionInsertMany(tree->left, item, amount);
		} else if (tree->elem < item) {
			tree->right = MsetUnionInsertMany(tree->right, item, amount);
		} 
		
		// If the item is already in the tree, makes the item's count
		// the higher count.
		else if (tree->elem == item) {
			if (amount >= tree->count) {
				tree->count = amount;
			}
		}

		return treeRebalance(tree);
	}
}

/**
* Returns a new multiset representing the intersection of the two
* given multisets.
*/
Mset MsetIntersection(Mset s1, Mset s2) {

	Mset msetIntersection = MsetNew();
	checkIntersection(msetIntersection, s1->tree, s2->tree);

	return msetIntersection;
}

// 
static Mset checkIntersection(Mset newMset, struct node *tree1,
								struct node *tree2) {

	if (tree1 == NULL || tree2 == NULL) {
		return newMset;
	}

	struct node *match = treeSearch(tree2, tree1->elem);

	if (match != NULL) {
		if (tree1->count < match->count) {
			MsetInsertMany(newMset, match->elem, tree1->count);
		} else {
			MsetInsertMany(newMset, match->elem, match->count);
		}
	}

	checkIntersection(newMset, tree1->right, tree2);
	checkIntersection(newMset, tree1->left, tree2);

	return newMset;
}

// Function searches the tree for a node with a matching element
// and if present, it returns it. Otherwise returns NULL.
// The following code was adapted from https://cgi.cse.unsw.edu.au/~cs2521/24T3/lectures/Week3Mon-bst.pdf.
static struct node *treeSearch(struct node *head, int item) {

	if (head == NULL) {
		return NULL;
	}

	if (head->elem == item) {
		struct node *match = head;
		return match;
	} else if (head->elem < item) {
		return treeSearch(head->right, item);
	} else if (head->elem > item) {
		return treeSearch(head->left, item);
	}

	return NULL;
}

/**
* Returns true if the multiset s1 is included in the multiset s2, and
* false otherwise.
*/
bool MsetIncluded(Mset s1, Mset s2) {

	bool result = checkIncluded(s1->tree, s2->tree);
	return result;
}

// Function checks if tree 1 is included in tree 2. Returns true if
// it is and false if it isn't.
static bool checkIncluded(struct node *tree1, struct node *tree2) {

	if (tree1 == NULL || tree2 == NULL) {
		return true;
	}

	// Returns true if tree 2 has the current tree 1 element and
	// if tree 2's matching element has a higher count than tree 1.
	// Otherwise, returns false.
	bool result = checkCounts(tree2, tree1->elem, tree1->count);

	// If result is false, Mset 1 is not included in Mset 2.
	if (!result) {
		return false;
	}

	return checkIncluded(tree1->right, tree2) &&
		checkIncluded(tree1->left, tree2);
}

// Function traverses the tree and returns true if it matches
// the MsetIncluded criteria. Otherwise, returns false.
static bool checkCounts(struct node *head, int item, int count) {

	if (head == NULL) {
		return false;
	}

	// If tree has a matching element to given item,
	// it checks if the count of the tree is greater than 
	// the given count.
	if (head->elem == item) {
		if (head->count >= count) {
			return true;
		} else {
			return false;
		}
	} else if (head->elem < item) {
		return checkCounts(head->right, item, count);
	} else if (head->elem > item) {
		return checkCounts(head->left, item, count);
	}

	return false;
}

/**
* Returns true if the two given multisets are equal, and false
* otherwise.
*/
bool MsetEquals(Mset s1, Mset s2) {

	bool result = checkEqual(s1->tree, s2->tree);

	return result;
}

// Function checks if tree 1 is equal to tree 2. Returns true if
// it is and false if it isn't.
static bool checkEqual(struct node *tree1, struct node *tree2) {

	if (tree1 == NULL || tree2 == NULL) {
		return true;
	}

	// Returns true if tree 2 has the current tree 1 element and
	// if tree 2's matching element has an equal count to tree 1.
	// Otherwise, returns false.
	bool result = checkEqualCounts(tree2, tree1->elem, tree1->count);

	if (!result) {
		return false;
	}

	return checkEqual(tree1->left, tree2) && checkEqual(tree1->right, tree2);
}

// Function traverses the tree and returns true if it matches
// the MsetEqual criteria. Otherwise, returns false.
static bool checkEqualCounts(struct node *head, int item, int count) {

	// Gets the node of the tree with equal item.
	struct node *match = treeSearch(head, item);

	// If the tree has a matching item, it checks
	// if the counts are equal. If so, returns true.
	// Otherwise, returns false.
	if (match != NULL) {
		if (count == match->count) {
			return true;
		} else {
			return false;
		}
	} 
	
	// If the tree does not have a matching item,
	// returns false.
	else {
		return false;
	}
}

/**
* Stores the k most common elements in the multiset into the given
* items array in decreasing order of count and returns the number of
* elements stored. Elements with the same count should be stored in
* increasing order. Assumes that the items array has size k.
*/
int MsetMostCommon(Mset s, int k, struct item items[]) {

	int count = storeInArray(items, s->tree, 0);
	
	// Arrange the array from most to least common.
	arrangeArray(items, count, k);

	// If the given k is higher than the amount of
	// elements in the multiset, return the element amount.
	if (k > count) {
		return count;
	} else {
		return k;
	}
}

// Function stores all mset elements into an item array. It also counts
// the amount of elements placed into the array and returns the count.
static int storeInArray(struct item items[], struct node *tree, int counter) {

	if (tree == NULL) {
		return counter;
	}

	struct item newItem;

	newItem.elem = tree->elem;
	newItem.count = tree->count;

	items[counter] = newItem;
	counter++;

	counter = storeInArray(items, tree->left, counter);
	counter = storeInArray(items, tree->right, counter);

	return counter;
}

// Function arranges the given array in order of highest to lowest count
// until the kth element.
static void arrangeArray(struct item arrayToOrder[], int count, int k) {

	if (k > count) {
		k = count;
	}

	for (int i = 0; i < k; i++) {
		for (int j = i + 1; j < count; j++) {
			if (arrayToOrder[i].count < arrayToOrder[j].count) {
				struct item temp = arrayToOrder[i];
				arrayToOrder[i] = arrayToOrder[j];
				arrayToOrder[j] = temp;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////
// Cursor Operations

/**
* Creates a new cursor positioned at the start of the multiset.
* (see spec for explanation of start and end)
*/
MsetCursor MsetCursorNew(Mset s) { 
	return NULL; 
}

/**
* Frees all memory allocated to the given cursor.
*/
void MsetCursorFree(MsetCursor cur) {
	return;
}

/**
* Returns the element at the cursor's position and its count, or
* {UNDEFINED, 0} if the cursor is positioned at the start or end of
* the multiset.
*/
struct item MsetCursorGet(MsetCursor cur) {
	return (struct item) {UNDEFINED, 0};
}

/**
* Moves the cursor to the next greatest element, or to the end of the
* multiset if there is no next greatest element. Does not move the
* cursor if it is already at the end. Returns false if the cursor is at
* the end after this operation, and true otherwise.
*/
bool MsetCursorNext(MsetCursor cur) { 
	return false; 
}

/**
* Moves the cursor to the next smallest element, or to the start of the
* multiset if there is no next smallest element. Does not move the
* cursor if it is already at the start. Returns false if the cursor is
* at the start after this operation, and true otherwise.
*/
bool MsetCursorPrev(MsetCursor cur) { 
	return false; 
}

////////////////////////////////////////////////////////////////////////