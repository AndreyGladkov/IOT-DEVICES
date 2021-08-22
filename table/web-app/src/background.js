chrome.runtime.onInstalled.addListener(() => {
    chrome.alarms.create('refresh', {
        periodInMinutes: 25,
    });
});

chrome.alarms.onAlarm.addListener(async ({ name }) => {
    if (name === 'refresh') {
        try {
            const socket = new WebSocket('ws://192.168.88.22:81');
            socket.addEventListener('open', () => {
                socket.send('TOGGLE');
            });
        } catch (error) {
            console.error(error);
        }
    }
});

chrome.runtime.onMessage.addListener((message) => {
    console.log(message);
});
