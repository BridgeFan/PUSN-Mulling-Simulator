#pragma once
//
// Created by kamil-hp on 27.03.23.
//

#ifndef MG1_ZAD2_OBJECTARRAYLISTENER_H
#define MG1_ZAD2_OBJECTARRAYLISTENER_H

namespace bf {
	class ObjectArray;
	class ObjectArrayListener {
	protected:
		ObjectArray& objectArray;
	public:
		virtual ~ObjectArrayListener();
		explicit ObjectArrayListener(ObjectArray& array);
		virtual void onRemoveObject(unsigned index)=0;
		virtual void onMoveObject(unsigned index)=0;
	};
}


#endif //MG1_ZAD2_OBJECTARRAYLISTENER_H
