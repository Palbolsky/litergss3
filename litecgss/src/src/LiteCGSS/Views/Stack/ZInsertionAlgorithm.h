#ifndef CGSS_Z_INSERTION_ALGORITHM_H
#define CGSS_Z_INSERTION_ALGORITHM_H

#include "ZAlgorithm.h"

namespace cgss {
	class ZInsertionAlgorithm : 
		public ZAlgorithm {

	public:
		using ZAlgorithm::ZAlgorithm;
		void add(GraphicsStackItemPtr item);
		void set(ZIndex& index, long z);
		bool remove(const GraphicsStackItem* item);
		void refresh();
	private:
		void zSwap(const ZIndex& previous, const ZIndex& next);
		void leftCircleShift(std::size_t begin, std::size_t end);
		void rightCircleShift(std::size_t begin, std::size_t end);
	};
}

#endif