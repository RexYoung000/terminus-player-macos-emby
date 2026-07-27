const test = require('node:test');
const assert = require('node:assert/strict');

const compatibility = require('../native/embyCompatibility');

function createWindow(response) {
    return {
        Response,
        location: {
            href: 'http://127.0.0.1:12345/index.html'
        },
        fetch: async () => response
    };
}

test('recognizes only the Quick Connect status endpoint', () => {
    assert.equal(
        compatibility.isQuickConnectStatus('https://media.example/QuickConnect/Status'),
        true
    );
    assert.equal(
        compatibility.isQuickConnectStatus('https://media.example/System/Info/Public'),
        false
    );
});

test('maps an unsupported Quick Connect endpoint to Unavailable', async () => {
    const target = createWindow(new Response('', { status: 404 }));
    assert.equal(compatibility.install(target), true);

    const response = await target.fetch('https://media.example/QuickConnect/Status');
    assert.equal(response.status, 200);
    assert.equal(await response.json(), 'Unavailable');
});

test('maps a missing Quick Connect enabled endpoint to false', async () => {
    const target = createWindow(new Response('', { status: 404 }));
    compatibility.install(target);

    const response = await target.fetch('https://media.example/QuickConnect/Enabled');
    assert.equal(response.status, 200);
    assert.equal(await response.json(), false);
});

test('does not hide failures from other server endpoints', async () => {
    const original = new Response('', { status: 404 });
    const target = createWindow(original);
    compatibility.install(target);

    const response = await target.fetch('https://media.example/Users/Public');
    assert.equal(response, original);
    assert.equal(response.status, 404);
});
