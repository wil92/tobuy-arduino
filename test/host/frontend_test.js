const assert = require("node:assert/strict");
const fs = require("node:fs");
const vm = require("node:vm");

class Element {
    constructor() {
        this.value = "";
        this.disabled = false;
        this.textContent = "";
        this.children = [];
        this.listeners = {};
        this.classList = {toggle: (name, enabled) => { this.error = enabled; }};
    }
    addEventListener(event, callback) { this.listeners[event] = callback; }
    replaceChildren() { this.children = []; }
    appendChild(child) { this.children.push(child); }
}

async function main() {
    const html = fs.readFileSync("frontend/setup.html", "utf8");
    const script = html.match(/<script>([\s\S]*?)<\/script>/)[1];
    const elements = Object.fromEntries(
        ["config", "ssid", "password", "networks", "status", "refresh", "save"]
            .map(id => [id, new Element()])
    );
    const requests = [];
    let scanPolls = 0;
    let saveStatus = 200;
    const fetch = async (url, options) => {
        requests.push({url, options});
        if (url === "/scan") return {ok: true, status: 202};
        if (url === "/refresh-ssid") {
            if (scanPolls++ === 0) return {ok: true, status: 202};
            return {ok: true, status: 200, json: async () => ['name,"<', "Other"]};
        }
        return {ok: saveStatus === 200, status: saveStatus, text: async () => "Saved"};
    };
    const document = {
        getElementById: id => elements[id],
        createElement: () => new Element()
    };
    vm.runInNewContext(script, {
        document, fetch, TextEncoder, URLSearchParams, setTimeout: callback => callback()
    });
    await new Promise(resolve => setImmediate(resolve));
    assert.deepEqual(elements.networks.children.map(item => item.value), ['name,"<', "Other"]);
    assert.equal(elements.refresh.disabled, false);

    const submit = elements.config.listeners.submit;
    elements.ssid.value = "network";
    elements.password.value = "short";
    await submit({preventDefault() {}});
    assert.equal(requests.filter(request => request.url === "/").length, 0);
    assert.equal(elements.status.error, true);

    elements.password.value = "p@ss word";
    saveStatus = 500;
    await submit({preventDefault() {}});
    assert.equal(elements.save.disabled, false);
    assert.equal(elements.status.error, true);
    const request = requests.find(item => item.url === "/");
    assert.equal(request.options.method, "POST");
    assert.equal(request.options.body.get("ssid"), "network");
    assert.equal(request.options.body.get("password"), "p@ss word");

    saveStatus = 200;
    elements.password.value = "";
    await submit({preventDefault() {}});
    assert.equal(elements.status.textContent, "Saved");
    assert.equal(elements.save.disabled, true);
}

main().catch(error => { console.error(error); process.exitCode = 1; });
