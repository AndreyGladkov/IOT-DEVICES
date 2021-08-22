import React, { useState, useLayoutEffect } from 'react';
import esp8266Socket from '../../modules/esp8266Socket';
import TableData from '../../modules/tableData';

import styles from './index.module.css';

const TablePosition = () => {
    const [height, setHeight] = useState<number>();
    const [min, setMin] = useState<number>(180);
    const [max, setMax] = useState<number>(6400);

    useLayoutEffect(() => {
        const onMessage = ({ data }: { data: string }) => {
            const { height, rangeMax, rangeMin }: TableData = JSON.parse(data);
            setHeight(height);
            setMax(rangeMax);
            setMin(rangeMin);
        };

        esp8266Socket.addEventListener('message', onMessage);

        return () => {
            esp8266Socket.removeEventListener('message', onMessage);
        };
    }, []);

    return (
        <div className={styles.tablePositionWrapper}>
            <input
                className={styles.tablePosition}
                type="range"
                id="table-position"
                name="table-position"
                min={min}
                max={max}
                value={height}
            />
        </div>
    );
};

export default TablePosition;
