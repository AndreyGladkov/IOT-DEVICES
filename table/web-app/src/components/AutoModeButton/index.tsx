import React, { useEffect, useState } from 'react';

import DigitalButton from '../DigitalButton';
import esp8266Socket from '../../modules/esp8266Socket';
import TableData, { AUTO_MODE } from '../../modules/tableData';

function AutoModeButton() {
    const [autoMode, setAutoMode] = useState<AUTO_MODE>();

    useEffect(() => {
        const onMessage = ({ data }: { data: string }) => {
            const { autoMode }: TableData = JSON.parse(data);
            setAutoMode(autoMode);
        };

        esp8266Socket.addEventListener('message', onMessage);

        return () => {
            esp8266Socket.removeEventListener('message', onMessage);
        };
    }, []);

    return (
        <DigitalButton
            isSelected={autoMode === AUTO_MODE.ON}
            onClick={() => {
                esp8266Socket.send(autoMode === AUTO_MODE.ON ? 'AUTO_MODE_OFF' : 'AUTO_MODE_ON');
            }}
        >
            auto
        </DigitalButton>
    );
}

export default AutoModeButton;
