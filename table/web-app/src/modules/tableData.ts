import DIRECTION from './direction';

export enum AUTO_MODE {
    OFF,
    ON,
}

export default interface TableData {
    direction: DIRECTION;
    autoMode: AUTO_MODE;
    height: number;
    rangeMax: number;
    rangeMin: number;
}
