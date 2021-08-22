import React, { useReducer, ReactNode } from 'react';

import FixedPositionContext, { initialState, reducer } from '../../modules/useFixedPositionControls';

const FixedPositionControlsContextProvider = ({ children }: { children: ReactNode }) => {
    const [state, dispatch] = useReducer(reducer, initialState);
    return <FixedPositionContext.Provider value={{ state, dispatch }}>{children}</FixedPositionContext.Provider>;
};

export default FixedPositionControlsContextProvider
