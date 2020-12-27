#ifndef CGSS_Z_ALGORITHM_H
#define CGSS_Z_ALGORITHM_H

#include "GraphicsContainer.h"
#include "LiteCGSS/Graphics/ZIndex.h"

namespace cgss {
	class ZAlgorithm {
	public:
		ZAlgorithm(GraphicsContainer& stack) : 
			m_stack(stack) {
		}
		ZAlgorithm(const ZAlgorithm&) = delete;
		ZAlgorithm& operator=(const ZAlgorithm&) = delete;
		ZAlgorithm(ZAlgorithm&&) = delete;
		ZAlgorithm& operator=(ZAlgorithm&&) = delete;
		virtual ~ZAlgorithm() = default;

		std::size_t findFirstGreater(const ZIndex& index) const;
		bool remove(const GraphicsStackItem* item);
	private:
		std::size_t dichotomicFindFirstGreater(const ZIndex& value) const;

	protected:
		GraphicsContainer& m_stack;
	};
}

#endif