#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "cs402.h"

#include "my402list.h"

int  My402ListLength(My402List* firstaddr)
{
	return firstaddr->num_members;
}


int  My402ListEmpty(My402List* firstaddr)
{
	if(firstaddr->num_members > 0)
	return FALSE;

	return TRUE;
}


int  My402ListAppend(My402List* firstaddr, void* data)
{
    // Create a space for an oject to be added
       
      My402ListElem* new_addr = (My402ListElem*)malloc(sizeof(My402ListElem));
      if(new_addr == NULL)
      return FALSE;

      firstaddr->num_members = firstaddr->num_members + 1;
      new_addr->obj = data;

	new_addr->prev =  firstaddr->anchor.prev;
        new_addr->next = &(firstaddr->anchor);
        firstaddr->anchor.prev = new_addr;
        new_addr->prev->next = new_addr;
        return TRUE;
}


int  My402ListPrepend(My402List* firstaddr, void* data)
{
      My402ListElem* new_addr = (My402ListElem*)malloc(sizeof(My402ListElem));
      if(new_addr == NULL)
      return FALSE;

      firstaddr->num_members = firstaddr->num_members + 1;
      new_addr->obj = data;

	new_addr->prev = &(firstaddr->anchor) ;
        new_addr->next = firstaddr->anchor.next;
        firstaddr->anchor.next = new_addr;
        new_addr->next->prev = new_addr;
        return TRUE;
}


void My402ListUnlink(My402List* firstaddr, My402ListElem* element)
{
	element->prev->next = element->next;
        element->next->prev = element->prev;
      firstaddr->num_members = firstaddr->num_members - 1;

	element->obj = NULL;
	element->prev = NULL;
	element->next = NULL;
	free(element);
}



void My402ListUnlinkAll(My402List * firstaddr)
{
        My402ListElem * temp = firstaddr->anchor.next;
	My402ListElem * unlink = NULL;
    while(temp!=NULL)
    {
        unlink = temp;
        temp = My402ListNext(firstaddr, temp);
        My402ListUnlink(firstaddr, unlink);
    }
	My402ListInit(firstaddr);
}


int  My402ListInsertAfter(My402List* firstaddr, void* data, My402ListElem* element)
{
	if(element == NULL)
	{
        int t = My402ListAppend(firstaddr, data);
	if(t == 1)
	return TRUE;
	else
	return FALSE;
	}   

	My402ListElem* new_addr = (My402ListElem*)malloc(sizeof(My402ListElem));
	if(new_addr == NULL)
	return FALSE;

      firstaddr->num_members = firstaddr->num_members + 1;
      new_addr->obj = data;

	new_addr->next = element->next;
	new_addr->prev = element;
	element->next = new_addr;
	new_addr->next->prev = new_addr;
	return TRUE;	     
}



int  My402ListInsertBefore(My402List* firstaddr, void* data, My402ListElem* element)
{
	if(element == NULL)
	{
        int t = My402ListPrepend(firstaddr, data);
	if(t == 1)
	return TRUE;
	else
	return FALSE;
	}   

	My402ListElem* new_addr = (My402ListElem*)malloc(sizeof(My402ListElem));
	if(new_addr == NULL)
	return FALSE;

      firstaddr->num_members = firstaddr->num_members + 1;
      new_addr->obj = data;

	new_addr->prev = element->prev;
	new_addr->next = element;
	element->prev = new_addr;
	new_addr->prev->next = new_addr;
	return TRUE;
}


My402ListElem *My402ListFirst(My402List* firstaddr)
{
	if(firstaddr->num_members == 0)
	return NULL;

	return firstaddr->anchor.next;
}


My402ListElem *My402ListLast(My402List* firstaddr)
{
	if(firstaddr->num_members == 0)
	return NULL;

	return firstaddr->anchor.prev;
}


My402ListElem *My402ListNext(My402List* firstaddr, My402ListElem* element)
{
	if(firstaddr->anchor.prev == element)
	return NULL;

	return element->next;
}


My402ListElem *My402ListPrev(My402List* firstaddr, My402ListElem* element)
{
	if(firstaddr->anchor.next == element)
	return NULL;

	return element->prev;
}

My402ListElem *My402ListFind(My402List* firstaddr , void* data)
{
	My402ListElem* temp = NULL;
	
	for(temp = My402ListFirst(firstaddr); temp!=NULL; temp = My402ListNext(firstaddr, temp))
	{
		if(temp->obj == data)
		return temp;
	}

	return NULL;
}

int My402ListInit(My402List* firstaddr)
{
	firstaddr->num_members = 0;
	firstaddr->anchor.obj = NULL;
	firstaddr->anchor.prev = &(firstaddr->anchor);
	firstaddr->anchor.next = &(firstaddr->anchor);
	return TRUE;
}
