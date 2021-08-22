import React, { useContext, useEffect, useState } from 'react';

import DigitalButton from '../DigitalButton/index';
import DIRECTION from '../../modules/direction';
import esp8266Socket from '../../modules/esp8266Socket';
import FixedPositionContext from '../../modules/useFixedPositionControls';

const sendCMD = (cmd: String | null) => {
    esp8266Socket.send(`${cmd?.toUpperCase()}`);
};

const DirectionButtons = () => {
    const [currentDirection, setDirection] = useState<DIRECTION>();
    const { state } = useContext(FixedPositionContext);

    useEffect(() => {
        esp8266Socket.addEventListener('message', ({ data }) => {
            const { direction }: { direction: DIRECTION } = JSON.parse(data);
            setDirection(direction);
        });
    }, []);

    const onClick = async ({ currentTarget }: React.FormEvent<HTMLButtonElement>) => {
        const cmd = currentTarget.dataset.name as DIRECTION;
        setDirection(cmd);

        if (state.isRecord && cmd !== DIRECTION.STOP) {
            await sendCMD(`RECORD_${cmd}`);
            setDirection(DIRECTION.STOP);
            return;
        }

        if (state.isMemory && cmd !== DIRECTION.STOP) {
            await sendCMD(`MEMORY_${cmd}`);
            return;
        }

        await sendCMD(cmd);
    };

    return (
        <>
            {Object.values(DIRECTION).map((item) => (
                <DigitalButton key={item} onClick={onClick} isSelected={currentDirection === item} data-name={item}>
                    {item.toLowerCase()}
                </DigitalButton>
            ))}
        </>
    );
};

export default DirectionButtons;
