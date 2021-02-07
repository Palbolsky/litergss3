#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Events/Patterns/EventDispatcher.h"
#include "Events/Patterns/SubObserver.h"

TEST(ObservableGroup, AddNotify) {
	int globalValue = 0;
	auto observer = cgss::Observer<int> {[&globalValue] (int& value) {
		globalValue = value;
		return true;
	}};

	auto observable = cgss::Observable<int> {};
	observable.addObserver(observer);

	int value = 3;
	observable.notifyObservers(value);

	EXPECT_EQ(globalValue, 3);
}

TEST(ObservableGroup, RemoveNotify) {
	int globalValue = 0;
	auto observer = cgss::Observer<int> {[&globalValue] (int& value) {
		globalValue = value;
		return true;
	}};

	auto observable = cgss::Observable<int> {};
	observable.addObserver(observer);

	int value = 3;
	observable.notifyObservers(value);

	value = 7;
	observable.removeObserver(observer);

	observable.notifyObservers(value);

	EXPECT_EQ(globalValue, 3);

	// Already removed : does not crash
	observable.removeObserver(observer);
}

TEST(ObservableGroup, AddSubNotify) {
	auto observable = cgss::Observable<int> {};
	int globalValue = 0;
	auto observer = cgss::SubObserver<int> {[&globalValue] (int& value) {
		globalValue = value;
		return true;
	}, observable};


	int value = 3;
	observable.notifyObservers(value);

	EXPECT_EQ(globalValue, 3);
}
