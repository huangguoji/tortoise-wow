#include "ActionBasket.h"

#pragma once
namespace ai
{
class Queue
{
public:
    Queue(void) {}
public:
    ~Queue(void) { RemoveExpired(); }
public:
	void Push(ActionBasket *action);
	ActionNode* Pop(ActionBasket* action = nullptr);
    ActionBasket* Peek();
	int Size();
	void RemoveExpired();
private:
	std::list<ActionBasket*> actions;
};
}
