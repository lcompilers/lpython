"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
const node_1 = require("vscode-languageserver/node");
const vscode_languageserver_textdocument_1 = require("vscode-languageserver-textdocument");
const vscode_languageserver_protocol_1 = require("vscode-languageserver-protocol");
// Create a connection for the server, using Node's IPC as a transport.
// Also include all preview / proposed LSP features.
const connection = (0, node_1.createConnection)(node_1.ProposedFeatures.all);
// Create a simple text document manager.
const documents = new node_1.TextDocuments(vscode_languageserver_textdocument_1.TextDocument);
let hasConfigurationCapability = false;
let hasWorkspaceFolderCapability = false;
let hasDiagnosticRelatedInformationCapability = false;
const fs = require("fs");
const tmp = require("tmp");
const path = require("path");
const util = require("node:util");
const node_util_1 = require("node:util");
// eslint-disable-next-line @typescript-eslint/no-var-requires
const exec = util.promisify(require('node:child_process').exec);
const tmpFile = tmp.fileSync();
function includeFlagForPath(file_path) {
    const protocol_end = file_path.indexOf("://");
    if (protocol_end == -1)
        return " -I " + file_path;
    // Not protocol.length + 3, include the last '/'
    return " -I " + path.dirname(file_path.slice(protocol_end + 2));
}
connection.onInitialize((params) => {
    const capabilities = params.capabilities;
    // Does the client support the `workspace/configuration` request?
    // If not, we fall back using global settings.
    hasConfigurationCapability = !!(capabilities.workspace && !!capabilities.workspace.configuration);
    hasWorkspaceFolderCapability = !!(capabilities.workspace && !!capabilities.workspace.workspaceFolders);
    hasDiagnosticRelatedInformationCapability = !!(capabilities.textDocument &&
        capabilities.textDocument.publishDiagnostics &&
        capabilities.textDocument.publishDiagnostics.relatedInformation);
    const result = {
        capabilities: {
            textDocumentSync: node_1.TextDocumentSyncKind.Incremental,
        }
    };
    if (hasWorkspaceFolderCapability) {
        result.capabilities.workspace = {
            workspaceFolders: {
                supported: true
            }
        };
    }
    console.log('LPython language server initialized');
    return result;
});
connection.onInitialized(() => {
    if (hasConfigurationCapability) {
        // Register for all configuration changes.
        connection.client.register(node_1.DidChangeConfigurationNotification.type, undefined);
    }
    if (hasWorkspaceFolderCapability) {
        // eslint-disable-next-line @typescript-eslint/no-unused-vars
        connection.workspace.onDidChangeWorkspaceFolders(_event => {
            connection.console.log('Workspace folder change event received.');
        });
    }
});
// The global settings, used when the `workspace/configuration` request is not supported by the client.
// Please note that this is not the case when using this server with the client provided in this example
// but could happen with other clients.
const defaultSettings = { maxNumberOfProblems: 1000, compiler: { executablePath: "/home/ankita/Documents/Internships/GSI/lpython/src/bin/lpython" } };
let globalSettings = defaultSettings;
// Cache the settings of all open documents
const documentSettings = new Map();
connection.onDidChangeConfiguration(change => {
    console.log("onDidChangeConfiguration, hasConfigurationCapability: " + hasConfigurationCapability);
    console.log("change is " + JSON.stringify(change));
    if (hasConfigurationCapability) {
        // Reset all cached document settings
        documentSettings.clear();
    }
    else {
        globalSettings = ((change.settings.LPythonLanguageServer || defaultSettings));
    }
    // Revalidate all open text documents
    documents.all().forEach(validateTextDocument);
});
// eslint-disable-next-line @typescript-eslint/no-unused-vars
function getDocumentSettings(resource) {
    if (!hasConfigurationCapability) {
        return Promise.resolve(globalSettings);
    }
    let result = documentSettings.get(resource);
    if (!result) {
        result = connection.workspace.getConfiguration({
            scopeUri: resource,
            section: 'LPythonLanguageServer'
        });
        documentSettings.set(resource, result);
    }
    return result;
}
// Only keep settings for open documents
documents.onDidClose(e => {
    documentSettings.delete(e.document.uri);
});
// eslint-disable-next-line @typescript-eslint/no-explicit-any
function throttle(fn, delay) {
    let shouldWait = false;
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    let waitingArgs;
    const timeoutFunc = () => {
        if (waitingArgs == null) {
            shouldWait = false;
        }
        else {
            fn(...waitingArgs);
            waitingArgs = null;
            setTimeout(timeoutFunc, delay);
        }
    };
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    return (...args) => {
        if (shouldWait) {
            waitingArgs = args;
            return;
        }
        fn(...args);
        shouldWait = true;
        setTimeout(timeoutFunc, delay);
    };
}
// The content of a text document has changed. This event is emitted
// when the text document first opened or when its content has changed.
documents.onDidChangeContent((() => {
    const throttledValidateTextDocument = throttle(validateTextDocument, 500);
    return (change) => {
        throttledValidateTextDocument(change.document);
    };
})());
async function runCompiler(text, flags, settings) {
    try {
        fs.writeFileSync(tmpFile.name, text);
    }
    catch (error) {
        console.log(error);
    }
    let stdout;
    try {
        const output = await exec(`/home/ankita/Documents/Internships/GSI/lpython/src/bin/lpython --show-errors ${tmpFile.name}`);
        // console.log(output);
        stdout = output.stdout;
        // eslint-disable-next-line @typescript-eslint/no-explicit-any
    }
    catch (e) {
        stdout = e.stdout;
        if (e.signal != null) {
            console.log("compile failed: ");
            console.log(e);
        }
        else {
            console.log("Error:", e);
        }
    }
    return stdout;
}
function findLineBreaks(utf16_text) {
    const utf8_text = new node_util_1.TextEncoder().encode(utf16_text);
    const lineBreaks = [];
    for (let i = 0; i < utf8_text.length; ++i) {
        if (utf8_text[i] == 0x0a) {
            lineBreaks.push(i);
        }
    }
    return lineBreaks;
}
async function validateTextDocument(textDocument) {
    console.time('validateTextDocument');
    if (!hasDiagnosticRelatedInformationCapability) {
        console.error('Trying to validate a document with no diagnostic capability');
        return;
    }
    // // In this simple example we get the settings for every validate run.
    const settings = await getDocumentSettings(textDocument.uri);
    // The validator creates diagnostics for all uppercase words length 2 and more
    const text = textDocument.getText();
    const lineBreaks = findLineBreaks(text);
    const stdout = await runCompiler(text, "--show-errors" + textDocument.uri, settings);
    const obj = JSON.parse(stdout);
    const diagnostics = [];
    if (obj.diagnostics) {
        const diagnostic = {
            severity: 2,
            range: {
                start: vscode_languageserver_protocol_1.Position.create(obj.diagnostics[0].range.start.line, obj.diagnostics[0].range.start.character),
                end: vscode_languageserver_protocol_1.Position.create(obj.diagnostics[0].range.end.line, obj.diagnostics[0].range.end.character),
            },
            message: obj.diagnostics[0].message,
            source: "lpyth"
        };
        diagnostics.push(diagnostic);
    }
    console.log(diagnostics);
    // Send the computed diagnostics to VSCode.
    connection.sendDiagnostics({ uri: textDocument.uri, diagnostics });
    console.timeEnd('validateTextDocument');
}
// eslint-disable-next-line @typescript-eslint/no-unused-vars
connection.onDidChangeWatchedFiles(_change => {
    // Monitored files have change in VSCode
    connection.console.log('We received an file change event');
});
// Make the text document manager listen on the connection
// for open, change and close text document events
documents.listen(connection);
// Listen on the connection
connection.listen();
//# sourceMappingURL=server.js.map