#ifndef __LOCKFREEQUEUE__
#define __LOCKFREEQUEUE__

#include "MemoryPool.h"



#define MAX 5000000
unsigned long long logindex = 0;
struct LogBox
{
	DWORD id;
	int type = 0xcccccccc;//0xbbbbbbbb Enqueue 0xdddddddd Dequeue
	int size = 0xcccccccc;
	int cookie = 0xcccccccc;
	void* curtail = nullptr;
	void* oldtail = nullptr;
	void* newtail = nullptr;
};

LogBox logbox[MAX];



template <class DATA>
class LFQueue
{
private:
	struct Node
	{
		DATA _data;
		Node* _next;
	};

public:
	LFQueue():_nodepool(0)
	{
		Node* dummy = _nodepool.Alloc();
		dummy->_next = nullptr;
		_head = dummy;
		_tail = dummy;
	}
	~LFQueue()
	{
		while (_head->_next!= nullptr)
		{
			Node* next = _head->_next;
			unsigned long long tempadr = (unsigned long long)_head;
			tempadr <<= 16;
			tempadr >>= 16;
			Node* realadr = (Node*)tempadr;
			delete realadr;
			_head = next;
		}
	}
	void Enqueue(DATA data)
	{
		Node* newnode = _nodepool.Alloc();
		newnode->_data = data;
		newnode->_next = nullptr;
		unsigned long long temp = InterlockedIncrement16(&_key);
		unsigned long long countnode = (unsigned long long)newnode;
		countnode |= (temp << 48);

		Node* oldtail;
		Node* realadr;
		do
		{
			while (1)
			{
				oldtail = _tail;
				unsigned long long tempadr = (unsigned long long)oldtail;
				tempadr <<= 16;
				tempadr >>= 16;
				realadr = (Node*)tempadr;

				if (realadr->_next != nullptr)
				{
					InterlockedCompareExchange64((__int64*)&_tail, (__int64)realadr->_next, (__int64)oldtail);
				}
				else
				{
					break;
				}
			}
		} while (InterlockedCompareExchange64((__int64*)&realadr->_next, (__int64)countnode, (__int64)nullptr) != (__int64)nullptr);
		unsigned long qsize = InterlockedIncrement(&_size);
		__int64 curtail=InterlockedCompareExchange64((__int64*)&_tail, (_int64)countnode, (__int64)oldtail);
		__int64 cmptail = curtail << 16;
		cmptail >>= 16;
		//if ((curtail != (__int64)oldtail)&&((__int64)realadr==cmptail))
		//{
		//	InterlockedCompareExchange64((__int64*)&_tail, (_int64)countnode, (__int64)_tail);
		//}
		unsigned long long index = InterlockedIncrement(&logindex);
		index %= MAX;
		logbox[index].id = GetCurrentThreadId();
		logbox[index].type = 0xbbbbbbbb;
		logbox[index].size = qsize;
		logbox[index].curtail = (Node*)curtail;
		logbox[index].oldtail = (Node*)oldtail;
		logbox[index].newtail = (Node*)countnode;

	}

	bool Dequeue(DATA* data)
	{
		Node* oldhead = nullptr;
		Node* newhead=nullptr;
		Node* realadr=nullptr;
		unsigned long long tempadr;
		unsigned long long tempadr2;
		do
		{
			if (_size != 0)
			{
				do
				{
					oldhead = _head;
					tempadr = (unsigned long long)oldhead;
					tempadr <<= 16;
					tempadr >>= 16;
					realadr = (Node*)tempadr;
					newhead = realadr->_next;
				} while (newhead == nullptr);
			}
			else
			{
				return false;
			}

			tempadr2 = (unsigned long long)newhead;
			tempadr2 <<= 16;
			tempadr2 >>= 16;
			Node* datanode = (Node*)tempadr2;
			*data = datanode->_data;
		} while (InterlockedCompareExchange64((__int64*)&_head,(__int64)newhead,(__int64)oldhead)!=(__int64)oldhead);
		unsigned long qsize=InterlockedDecrement(&_size);

		_nodepool.Free(realadr);
		unsigned long long index = InterlockedIncrement(&logindex);
		index %= MAX;
		logbox[index].id = GetCurrentThreadId();
		logbox[index].type = 0xdddddddd;
		logbox[index].size = qsize;
		logbox[index].oldtail = (Node*)oldhead;
		logbox[index].newtail = (Node*)newhead;
		return true;
	}


private:
	Node* _head;
	Node* _tail;
	MemoryPool<Node> _nodepool;
	
	unsigned long _size=0;
	short _key = 0;
};





#endif
