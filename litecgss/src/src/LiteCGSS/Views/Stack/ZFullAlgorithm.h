#ifndef CGSS_Z_FULL_ALGORITHM_H
#define CGSS_Z_FULL_ALGORITHM_H

#include "ZAlgorithm.h"

namespace cgss {
	class ZFullAlgorithm : 
		private ZAlgorithm {

	public:
		using ZAlgorithm::ZAlgorithm;
		void add(GraphicsStackItemPtr item);
		void set(ZIndex& index, long z);
		bool remove(const GraphicsStackItem* item);
		void refresh();
	private:
		void sortZ();
		void setZDirty();
		bool m_zDirty = false;
	};
}

#endif