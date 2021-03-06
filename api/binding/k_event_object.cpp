/*
 * Appcelerator Kroll - licensed under the Apache Public License 2
 * see LICENSE in the root folder for details on the license.
 * Copyright (c) 2008 Appcelerator, Inc. All Rights Reserved.
 */

#include "../kroll.h"
namespace kroll
{
	unsigned int EventListener::currentId = 1;
	std::map<KEventObject*, std::vector<EventListener*>*> KEventObject::listenerMap;
	Poco::Mutex KEventObject::listenerMapMutex;
	AutoPtr<KEventObject> KEventObject::root = new KEventObject(true, "Host");

	KEventObject::KEventObject(const char *type) :
		AccessorBoundObject(type),
		isRoot(false)
	{
		{
			Poco::Mutex::ScopedLock lock(listenerMapMutex);
			std::vector<EventListener*>* listeners = new std::vector<EventListener*>();
			KEventObject::listenerMap[this] = listeners;
		}

		this->SetMethod("addEventListener", &KEventObject::_AddEventListener);
		this->SetMethod("removeEventListener", &KEventObject::_RemoveEventListener);
	}

	KEventObject::KEventObject(bool isRoot, const char *type) :
		AccessorBoundObject(type),
		isRoot(isRoot)
	{
		{
			Poco::Mutex::ScopedLock lock(listenerMapMutex);
			std::vector<EventListener*>* listeners = new std::vector<EventListener*>();
			KEventObject::listenerMap[this] = listeners;
		}

		this->SetMethod("addEventListener", &KEventObject::_AddEventListener);
		this->SetMethod("removeEventListener", &KEventObject::_RemoveEventListener);
	}

	KEventObject::~KEventObject()
	{
		Poco::Mutex::ScopedLock lock(listenerMapMutex);
		std::vector<EventListener*>* listeners = KEventObject::listenerMap[this];
		std::vector<EventListener*>::iterator i = listeners->begin();
		while (i != listeners->end())
		{
			EventListener* listener = *i++;
			delete listener;
		}
		listeners->clear();

		KEventObject::listenerMap.erase(this);
		delete listeners;
	}

	void KEventObject::RemoveEventListener(std::string& eventName, SharedKMethod listener)
	{
		this->RemoveEventListener(eventName, 0, listener);
	}

	void KEventObject::RemoveEventListener(std::string& eventName, unsigned int listenerId)
	{
		this->RemoveEventListener(eventName, listenerId, 0);
	}

	void KEventObject::RemoveEventListener(
		std::string& eventName, unsigned int listenerId, SharedKMethod callback)
	{
		Poco::Mutex::ScopedLock lock(listenerMapMutex);
		std::vector<EventListener*>* listeners = KEventObject::listenerMap[this];
		std::vector<EventListener*>::iterator i = listeners->begin();
		while (i != listeners->end())
		{
			EventListener* listener = *i;
			if (listener->Matches(eventName, listenerId, callback))
			{
				i = listeners->erase(i);
				delete listener;
			}
			else
			{
				i++;
			}
		}
	}

	unsigned int KEventObject::AddEventListener(
		std::string& eventName, SharedKMethod callback)
	{
		Poco::Mutex::ScopedLock lock(listenerMapMutex);
		EventListener* listener = new EventListener(eventName, callback);
		std::vector<EventListener*>* listeners = KEventObject::listenerMap[this];
		listeners->push_back(listener);
		return listener->listenerId;
	}

	unsigned int KEventObject::AddEventListenerForAllEvents(SharedKMethod callback)
	{
		Poco::Mutex::ScopedLock lock(listenerMapMutex);
		EventListener* listener = new EventListener(Event::ALL, callback);
		std::vector<EventListener*>* listeners = KEventObject::listenerMap[this];
		listeners->push_back(listener);
		return listener->listenerId;
	}

	void KEventObject::FireRootEvent(std::string& eventName)
	{
		KEventObject::root->FireEvent(eventName);
	}

	void KEventObject::FireRootEvent(AutoPtr<Event>event)
	{
		KEventObject::root->FireEvent(event);
	}

	void KEventObject::FireEvent(std::string& eventName)
	{
		this->duplicate();
		AutoPtr<KEventObject> target = this;
		AutoPtr<Event> event = new Event(target, eventName);
		this->FireEvent(event);
	}

	void KEventObject::FireEvent(AutoPtr<Event> event)
	{
		std::vector<EventListener*> listeners;
		{
			// We cannot block any threads wile we handle callbacks, so
			// we must make a copy of this map while we handle callbacks. Always
			// remember: premature optimization is the root of all evil.
			Poco::Mutex::ScopedLock lock(listenerMapMutex);
			listeners = *(KEventObject::listenerMap[this]);
		}

		std::vector<EventListener*>::iterator li = listeners.begin();
		while (li != listeners.end())
		{
			EventListener* listener = *li++;
			listener->FireEventIfMatches(event);

			if (event->stopped)
				return;
		}

		if (!this->isRoot)
			KEventObject::root->FireEvent(event);
	}

	void KEventObject::_AddEventListener(const ValueList& args, SharedValue result)
	{
		unsigned int listenerId;
		if (args.size() > 1 && args.at(0)->IsString() && args.at(1)->IsMethod()) {
			std::string eventName = args.GetString(0);
			listenerId = this->AddEventListener(eventName, args.GetMethod(1));

		} else if (args.size() > 0 && args.at(0)->IsMethod()) {
			listenerId = this->AddEventListenerForAllEvents(args.GetMethod(0));

		} else {
			throw ValueException::FromString("Incorrect arguments passed to addEventListener");
		}

		result->SetDouble((double) listenerId);
	}

	void KEventObject::_RemoveEventListener(const ValueList& args, SharedValue result)
	{
		args.VerifyException("removeEventListener", "s n|m");

		std::string eventName = args.GetString(0);
		if (args.at(1)->IsMethod()) {
			this->RemoveEventListener(eventName, args.GetMethod(1));
		} else {
			this->RemoveEventListener(eventName, args.GetInt(1));
		}
	}

	void EventListener::FireEventIfMatches(AutoPtr<Event> event)
	{
		if (event->eventName != this->eventName && this->eventName != Event::ALL)
			return;

		try
		{
			Host* host = Host::GetInstance();
			host->InvokeMethodOnMainThread(
				callback, ValueList(Value::NewObject(event)));
		}
		catch (ValueException& e)
		{
			Logger* logger = Logger::Get("KEventObject");
			SharedString ss = e.DisplayString();
			logger->Error("Exception caught during window event callback: %s", ss->c_str());

		}
	}

	bool EventListener::Matches(
		std::string& eventName, unsigned int listenerId, SharedKMethod callback)
	{
		return eventName == eventName &&
			((callback.isNull() && listenerId == 0) ||
			(!callback.isNull() && callback->Equals(this->callback)) ||
			(listenerId != 0 && listenerId == this->listenerId));
	}
}

