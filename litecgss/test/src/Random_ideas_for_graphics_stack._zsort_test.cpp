#include <doctest.h>

#include <iostream>
#include <functional>
#include <optional>
#include <algorithm>
#include <chrono>
#include <vector>
#include <climits>

struct ZIndex {
    long z = 0;
    long index = 0;

    int sign() const {
      long signZ = (z != 0) | -(long)((unsigned long)((long)z) >> (sizeof(long) * CHAR_BIT - 1));
      if (signZ != 0) {
        return signZ;
      }
      long signIndex = (index != 0) | -(long)((unsigned long)((long)index) >> (sizeof(long) * CHAR_BIT - 1));
      return signIndex;
    }

    ZIndex operator+(const ZIndex& alter) const {
        return {z + alter.z, index + alter.index};
    }

    ZIndex operator-(const ZIndex& alter) const {
        return {z - alter.z, index - alter.index};
    }

    bool operator==(const ZIndex& alter) const {
        return z == alter.z && index == alter.index;
    }
    
    bool operator>(const ZIndex& alter) const {
        if (z > alter.z) {
            return true;
        }
        return z == alter.z && index > alter.index;
    }

    bool operator<=(const ZIndex& alter) const {
        return !operator>(alter);
    }
    
    bool operator<(const ZIndex& alter) const {
        if (z < alter.z) {
            return true;
        }
        return z == alter.z && index < alter.index;
    }

    bool operator>=(const ZIndex& alter) const {
        return !operator<(alter);
    }
};


// What this function does :
// RightCircleShift(1, 3, [A, B, C, D, E]) => [A, D, B, C, E]
// RightCircleShift(0, 5, [A, B, C, D, E]) => [E, A, B, C, D]
// Obviously we suppose there is not any out of bounds array access when calling this
static void RightCircleShift(std::size_t begin, std::size_t end, std::vector<ZIndex>& container) {
    auto last = std::move(container[end]);
    for(auto index = end; index > begin; index--) {
        container[index] = std::move(container[index - 1]);
    }
    container[begin] = std::move(last);
}


// The opposite of RightCircleShift
static void LeftCircleShift(std::size_t begin, std::size_t end, std::vector<ZIndex>& container) {
    auto first = std::move(container[begin]);
    for(auto index = begin; index < end; index++) {
        container[index] = std::move(container[index + 1]);
    }
    container[end] = std::move(first);
}

// 1
static std::size_t ZSort(ZIndex& editedElement, std::vector<ZIndex>& container) {
    std::size_t range[2] = {std::numeric_limits<std::size_t>::max(), std::numeric_limits<std::size_t>::max()};
    std::size_t index;
    for (index = 0; index < container.size() && !(container[index] > editedElement); index++) {
        if (container[index] == editedElement) {
            range[0] = index;
        }
    }
    
    if (index == container.size()) {
      return index - 1;
    }

    if (range[0] != std::numeric_limits<std::size_t>::max()) {
        range[1] = index - 1;
        LeftCircleShift(range[0], range[1], container);
          //std::cout << "Range : " << range[0] << ", " <<  range[1] << std::endl;
        return range[1];
    }

    range[0] = index;
    for (; index < container.size() && !(container[index] == editedElement); index++);
    range[1] = index;
    RightCircleShift(range[0], range[1], container);

    //std::cout << "Range : " << range[0] << ", " <<  range[1] << std::endl;
    return range[0];    
}

// 2
static std::size_t DichotomicFindFirstGreater(const ZIndex& value, std::vector<ZIndex>& container) {
  if (container.empty()) {
    return std::numeric_limits<std::size_t>::max();
  }

  auto min = 0;
  auto max = container.size() - 1;
  while (max - min > 1) {
    const auto index = (max + min) >> 1;
    const auto diff = (value - container[index]).sign();
    //std::cout << "Diff : " << value.z << " - " << container[index].z << " = " << diff << " for index " << index << std::endl;
    switch (diff) {
      case 0:
      return index;
      case 1:
      min = index;
      break;
      case -1:
      max = index;
      break;
    }
  }
  return max;
}

static std::size_t DichotomicZSortInsert(ZIndex& previous, ZIndex next, std::vector<ZIndex>& container) {
    /*
    if (previous == next) {
      //TODO : return previous index
      return ;
    }
    */

    std::size_t range[2];
    //std::cout << "previous" << std::endl;
    range[0] = DichotomicFindFirstGreater(previous, container);
    //std::cout << "next" << std::endl;
    range[1] = DichotomicFindFirstGreater(next, container);
    
    const auto diff = static_cast<long>(range[1] - range[0]);
    if (diff == 0) {
      return range[0];
    }

    if (diff > 0) {
        range[1]--;
        LeftCircleShift(range[0], range[1], container);
    } else {
        RightCircleShift(range[1], range[0], container);
    }
    container[range[1]] = std::move(next);

    //std::cout << "Range : " << range[0] << ", " <<  range[1] << std::endl;

    return range[1];
}

//////////////////////////////////////////////////////////////////////////////////////////////

static void ZPrint(const std::vector<ZIndex>& container) {
    std::cout << "[ ";
    for (const auto& element : container) {
        std::cout << element.z << "." << element.index << ", ";
    }
    std::cout << "]" << std::endl;
}

static long long BenchMicroSec(std::function<void()> toCall, std::size_t iterations = 500000) {
    auto t1 = std::chrono::high_resolution_clock::now();
    for(std::size_t i = 0; i < iterations; i++) {
      toCall();
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();

    return duration;
}


static void Benchmarks(const std::vector<ZIndex>& chosenStack) {
  auto stack = chosenStack;
  
  
  auto durationZSort = BenchMicroSec([&stack]() {
    std::size_t nextIndex = 6;
    stack[nextIndex].z = 2;
    nextIndex = ZSort(stack[nextIndex], stack);

    stack[nextIndex].z = 3;
    nextIndex = ZSort(stack[nextIndex], stack);
    
    stack[nextIndex].z = 7;
    nextIndex = ZSort(stack[nextIndex], stack);
      
    stack[nextIndex].z = 12;
    nextIndex = ZSort(stack[nextIndex], stack);

    stack[nextIndex].z = 0;
    nextIndex = ZSort(stack[nextIndex], stack);
  });

  stack = chosenStack;
  
  ZIndex prev;
  auto durationDichotomicZSort = BenchMicroSec([&stack, &prev]() {
    std::size_t nextIndex = 6;
    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {2, prev.index}, stack);

    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {3, prev.index}, stack);

    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {7, prev.index}, stack);

    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {12, prev.index}, stack);

    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {0, prev.index}, stack);
  });

  stack = chosenStack;

  auto durationClassicSort = BenchMicroSec([&stack]() {
    stack[6].z = 2;
    std::sort(stack.begin(), stack.end());

    stack[4].z = 3;
    std::sort(stack.begin(), stack.end());
    
    stack[5].z = 7;
    std::sort(stack.begin(), stack.end());
      
    stack[12].z = 0;
    std::sort(stack.begin(), stack.end());

    stack[2].z = 0;
    std::sort(stack.begin(), stack.end());
  });

  std::cout << "DichotomicZSort : " << durationDichotomicZSort / 1000.0 << " ms" << std::endl;
  std::cout << "ZSort : " << durationZSort / 1000.0 << " ms" << std::endl;
  std::cout << "std sort : " << durationClassicSort / 1000.0 << " ms" << std::endl;
}

static void BenchmarksBigStackStart(const std::vector<ZIndex>& chosenStack) {
  auto stack = chosenStack;
  auto durationZSort = BenchMicroSec([&stack]() {
    std::size_t nextIndex = 6;
    stack[nextIndex].z = 2;
    nextIndex = ZSort(stack[nextIndex], stack);

    stack[nextIndex].z = 3;
    nextIndex = ZSort(stack[nextIndex], stack);
    
    stack[nextIndex].z = 7;
    nextIndex = ZSort(stack[nextIndex], stack);
      
    stack[nextIndex].z = 12;
    nextIndex = ZSort(stack[nextIndex], stack);

    stack[nextIndex].z = 0;
    nextIndex = ZSort(stack[nextIndex], stack);
  }, 5000);
  std::cout << "ZSort : " << durationZSort / 1000.0 << " ms" << std::endl;
  stack = chosenStack;
  
  ZIndex prev;
  auto durationDichotomicZSort = BenchMicroSec([&stack, &prev]() {
    std::size_t nextIndex = 6;
    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {2, prev.index}, stack);

    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {3, prev.index}, stack);

    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {7, prev.index}, stack);

    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {12, prev.index}, stack);

    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {0, prev.index}, stack);
  }, 5000);
  std::cout << "DichotomicZSort : " << durationDichotomicZSort / 1000.0 << " ms" << std::endl;
  /*stack = chosenStack;

  auto durationClassicSort = BenchMicroSec([&stack]() {
    stack[6].z = 2;
    std::sort(stack.begin(), stack.end());

    stack[4].z = 3;
    std::sort(stack.begin(), stack.end());
    
    stack[5].z = 7;
    std::sort(stack.begin(), stack.end());
      
    stack[12].z = 0;
    std::sort(stack.begin(), stack.end());

    stack[2].z = 0;
    std::sort(stack.begin(), stack.end());
  }, 5000);
  
  std::cout << "std sort : " << durationClassicSort / 1000.0 << " ms" << std::endl;*/
}

static void BenchmarksBigStackEnd(const std::vector<ZIndex>& chosenStack) {
  auto stack = chosenStack;
  auto durationZSort = BenchMicroSec([&stack]() {
    std::size_t nextIndex = stack.size() - 20;
    stack[nextIndex].z = 99999000;
    nextIndex = ZSort(stack[nextIndex], stack);

    stack[nextIndex].z = 99999010;
    nextIndex = ZSort(stack[nextIndex], stack);
    
    stack[nextIndex].z = 99999525;
    nextIndex = ZSort(stack[nextIndex], stack);
      
    stack[nextIndex].z = 99999111;
    nextIndex = ZSort(stack[nextIndex], stack);

    stack[nextIndex].z = 99999387;
    nextIndex = ZSort(stack[nextIndex], stack);
  }, 5000);
  std::cout << "ZSort : " << durationZSort / 1000.0 << " ms" << std::endl;
  stack = chosenStack;
  
  ZIndex prev;
  auto durationDichotomicZSort = BenchMicroSec([&stack, &prev]() {
    std::size_t nextIndex = stack.size() - 20;
    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {99999000, prev.index}, stack);

    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {99999010, prev.index}, stack);

    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {99999525, prev.index}, stack);

    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {99999111, prev.index}, stack);

    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {99999387, prev.index}, stack);
  }, 5000);
  std::cout << "DichotomicZSort : " << durationDichotomicZSort / 1000.0 << " ms" << std::endl;
  stack = chosenStack;
}

static void BenchmarksBigStackRandom(const std::vector<ZIndex>& chosenStack) {
    static constexpr const auto Iterations = 5000;

  auto stack = chosenStack;
  auto durationZSort = BenchMicroSec([&stack]() {
    std::size_t nextIndex = stack.size() - 20;
    stack[nextIndex].z = 23339360;
    nextIndex = ZSort(stack[nextIndex], stack);

    stack[nextIndex].z = 53917409;
    nextIndex = ZSort(stack[nextIndex], stack);
    
    stack[nextIndex].z = 25074684;
    nextIndex = ZSort(stack[nextIndex], stack);
      
    stack[nextIndex].z = 65843891;
    nextIndex = ZSort(stack[nextIndex], stack);

    stack[nextIndex].z = 58960469;
    nextIndex = ZSort(stack[nextIndex], stack);
  }, Iterations);
  std::cout << "ZSort : " << durationZSort / 1000.0 << " ms" << std::endl;
  stack = chosenStack;
  
  ZIndex prev;
  auto durationDichotomicZSort = BenchMicroSec([&stack, &prev]() {
    std::size_t nextIndex = stack.size() - 20;
    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {23339360, prev.index}, stack);

    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {53917409, prev.index}, stack);

    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {25074684, prev.index}, stack);

    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {65843891, prev.index}, stack);

    prev = stack[nextIndex];
    nextIndex = DichotomicZSortInsert(prev, {58960469, prev.index}, stack);
  }, Iterations);
  std::cout << "DichotomicZSort : " << durationDichotomicZSort / 1000.0 << " ms" << std::endl;
  stack = chosenStack;
}

static const auto MaxStackNum = std::numeric_limits<long>::max();

static std::vector<ZIndex> BuildBigStack() {
  auto result = std::vector<ZIndex>{};

  for (std::size_t i = 0; i < 10000; i++) {
    result.push_back({static_cast<long>(i*i), static_cast<long>(i%20)});
  }

  return result;
}

static const auto SmallStack = std::vector<ZIndex> { {0, 0}, {0, 15}, {1, 1}, {1, 2}, {2, 33}, {3, 55}, {3, 22}, {4, 66}, {5, 3}, {6, 4}, {MaxStackNum, 5}, {MaxStackNum, 6}, {MaxStackNum, 7} };

#ifdef COMPILE_BENCH

TEST_CASE("[Graphics stack z sorting] Benchmark big array start") {
    BenchmarksBigStackStart(BuildBigStack());
}

TEST_CASE("[Graphics stack z sorting] Benchmark  big array start") {
    BenchmarksBigStackEnd(BuildBigStack());
}

TEST_CASE("[Graphics stack z sorting] Benchmark  big array random") {
    BenchmarksBigStackRandom(BuildBigStack());
}

#endif

