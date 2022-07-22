"use strict";
/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
Object.defineProperty(exports, "__esModule", { value: true });
exports.setTestContent = exports.getDocUri = exports.getDocPath = exports.activate = exports.platformEol = exports.documentEol = exports.editor = exports.doc = void 0;
const vscode = require("vscode");
const path = require("path");
/**
 * Activates the vscode.lsp-sample extension
 */
async function activate(docUri) {
    // The extensionId is `publisher.name` from package.json
    const ext = vscode.extensions.getExtension('vscode-samples.lsp-sample');
    await ext.activate();
    try {
        exports.doc = await vscode.workspace.openTextDocument(docUri);
        exports.editor = await vscode.window.showTextDocument(exports.doc);
        await sleep(2000); // Wait for server activation
    }
    catch (e) {
        console.error(e);
    }
}
exports.activate = activate;
async function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}
const getDocPath = (p) => {
    return path.resolve(__dirname, '../../testFixture', p);
};
exports.getDocPath = getDocPath;
const getDocUri = (p) => {
    return vscode.Uri.file((0, exports.getDocPath)(p));
};
exports.getDocUri = getDocUri;
async function setTestContent(content) {
    const all = new vscode.Range(exports.doc.positionAt(0), exports.doc.positionAt(exports.doc.getText().length));
    return exports.editor.edit(eb => eb.replace(all, content));
}
exports.setTestContent = setTestContent;
//# sourceMappingURL=helper.js.map