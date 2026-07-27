(function(root, factory) {
    const compatibility = factory();

    if (typeof module === 'object' && module.exports) {
        module.exports = compatibility;
    } else {
        root.TerminusEmbyCompatibility = compatibility;
    }
}(typeof self !== 'undefined' ? self : this, function() {
    function requestUrl(input) {
        if (typeof input === 'string') {
            return input;
        }

        return input && input.url ? input.url : '';
    }

    function quickConnectFallback(url, baseUrl) {
        try {
            const parsed = new URL(url, baseUrl || 'http://127.0.0.1/');
            if (/^\/quickconnect\/enabled\/?$/i.test(parsed.pathname)) {
                return false;
            }
            if (/^\/quickconnect\/status\/?$/i.test(parsed.pathname)) {
                return 'Unavailable';
            }
        } catch (error) {
            return undefined;
        }
        return undefined;
    }

    function isQuickConnectStatus(url, baseUrl) {
        return quickConnectFallback(url, baseUrl) === 'Unavailable';
    }

    function install(target) {
        if (!target || typeof target.fetch !== 'function' ||
            target.__terminusEmbyCompatibilityInstalled) {
            return false;
        }

        const originalFetch = target.fetch.bind(target);
        target.fetch = async function(input, init) {
            const response = await originalFetch(input, init);
            const url = requestUrl(input);
            const fallback = quickConnectFallback(
                url,
                target.location && target.location.href
            );

            if ((response.status === 404 || response.status === 405) &&
                fallback !== undefined) {
                console.info('Quick Connect is unavailable on this server.');
                return new target.Response(JSON.stringify(fallback), {
                    status: 200,
                    headers: {
                        'Content-Type': 'application/json'
                    }
                });
            }

            return response;
        };

        target.__terminusEmbyCompatibilityInstalled = true;
        return true;
    }

    return {
        install,
        isQuickConnectStatus,
        quickConnectFallback
    };
}));
