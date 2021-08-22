import React from 'react';

export enum FIXED_POSITION {
    RECORD = 'FIXED_POSITION_RECORD_ACTION',
    MEMORY = 'FIXED_POSITION_MEMORY_ACTION',
}

type State = {
    isRecord: boolean;
    isMemory: boolean;
};

type Action = {
    type: FIXED_POSITION;
    payload: boolean;
};

export const reducer = (state: State, action: Action) => {
    switch (action.type) {
        case FIXED_POSITION.RECORD:
            return { isMemory: action.payload && !action.payload, isRecord: action.payload };
        case FIXED_POSITION.MEMORY:
            return { isMemory: action.payload, isRecord: action.payload && !action.payload };
        default:
            return state;
    }
};

export const initialState = { isRecord: false, isMemory: true };

export default React.createContext({ state: initialState, dispatch: (action: Action) => {} });
