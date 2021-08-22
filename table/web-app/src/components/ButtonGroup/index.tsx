import { ReactNode } from 'react';

import styles from './index.module.css';

const ButtonGroup = ({ children }: { children: ReactNode }) => {
    return <div className={styles.buttonGroup}>{children}</div>;
};

export default ButtonGroup;
