import React, { useContext } from 'react';

import DigitalButton from '../DigitalButton';
import FixedPositionContext, { FIXED_POSITION } from '../../modules/useFixedPositionControls';
import ButtonGroup from '../ButtonGroup';

import styles from './index.module.css';

const FixedPositionControls = () => {
    const { state, dispatch } = useContext(FixedPositionContext);

    return (
        <>
            <ButtonGroup>
                <DigitalButton
                    onClick={() => {
                        dispatch({ type: FIXED_POSITION.RECORD, payload: !state.isRecord });
                    }}
                    isSelected={state.isRecord}
                >
                    record
                </DigitalButton>
            </ButtonGroup>
            <div>
                <div className={state.isRecord || state.isMemory ? styles.hint : styles.hintHide}>
                    {state.isRecord && 'press the up/down button to memorize the current position'}
                    {state.isMemory && 'press the up/down button to use fixed position'}
                </div>
            </div>
            <ButtonGroup>
                <DigitalButton
                    onClick={() => {
                        dispatch({ type: FIXED_POSITION.MEMORY, payload: !state.isMemory });
                    }}
                    isSelected={state.isMemory}
                >
                    fixed
                </DigitalButton>
            </ButtonGroup>
        </>
    );
};

export default FixedPositionControls;
