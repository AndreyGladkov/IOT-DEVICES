import React from 'react';

import styles from './index.module.css';

interface ButtonProps {
    children: React.ReactNode;
    isSelected: boolean;
    onClick: ({ currentTarget }: React.FormEvent<HTMLButtonElement>) => void;
}

const DigitalButton = ({ children, onClick, isSelected, ...rest }: ButtonProps) => (
    <div className={styles.buttonWrapper}>
        <button {...rest} className={isSelected ? styles.buttonPressed : styles.button} type="button" onClick={onClick}>
            <span />
        </button>
        {children}
    </div>
);

export default DigitalButton;
