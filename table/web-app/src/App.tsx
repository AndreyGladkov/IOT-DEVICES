import React from 'react';

import DirectionButtons from './components/DirectionButtons';
import AutoModeButton from './components/AutoModeButton';
import FixedPositionControls from './components/FixedPositionControls';
import FixedPositionControlsContextProvider from './components/FixedPositionControls/Context';
import TablePosition from './components/TablePosition';
import ButtonGroup from './components/ButtonGroup';

import style from './App.module.css';

function App() {
    return (
        <FixedPositionControlsContextProvider>
            <div className={style.appContainer}>
                <TablePosition />
                <div className={style.appPanel}>
                    <div className={style.appRowTop}>
                        <FixedPositionControls />
                    </div>
                    <div className={style.appRow}>
                        <ButtonGroup>
                            <DirectionButtons />
                        </ButtonGroup>
                        <div className={style.appGapLeft}>
                            <ButtonGroup>
                                <AutoModeButton />
                            </ButtonGroup>
                        </div>
                    </div>
                </div>
            </div>
        </FixedPositionControlsContextProvider>
    );
}

export default App;
