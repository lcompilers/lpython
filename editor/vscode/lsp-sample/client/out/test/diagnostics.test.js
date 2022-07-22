"use strict";
/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
Object.defineProperty(exports, "__esModule", { value: true });
const vscode = require("vscode");
const assert = require("assert");
const helper_1 = require("./helper");
suite('Should get diagnostics', () => {
    const docUri = (0, helper_1.getDocUri)('diagnostics.txt');
    test('Diagnoses uppercase texts', async () => {
        await testDiagnostics(docUri, [
            { message: 'ANY is all uppercase.', range: toRange(0, 0, 0, 3), severity: vscode.DiagnosticSeverity.Warning, source: 'ex' },
            { message: 'ANY is all uppercase.', range: toRange(0, 14, 0, 17), severity: vscode.DiagnosticSeverity.Warning, source: 'ex' },
            { message: 'OS is all uppercase.', range: toRange(0, 18, 0, 20), severity: vscode.DiagnosticSeverity.Warning, source: 'ex' }
        ]);
    });
});
function toRange(sLine, sChar, eLine, eChar) {
    const start = new vscode.Position(sLine, sChar);
    const end = new vscode.Position(eLine, eChar);
    return new vscode.Range(start, end);
}
async function testDiagnostics(docUri, expectedDiagnostics) {
    await (0, helper_1.activate)(docUri);
    const actualDiagnostics = vscode.languages.getDiagnostics(docUri);
    assert.equal(actualDiagnostics.length, expectedDiagnostics.length);
    expectedDiagnostics.forEach((expectedDiagnostic, i) => {
        const actualDiagnostic = actualDiagnostics[i];
        assert.equal(actualDiagnostic.message, expectedDiagnostic.message);
        assert.deepEqual(actualDiagnostic.range, expectedDiagnostic.range);
        assert.equal(actualDiagnostic.severity, expectedDiagnostic.severity);
    });
}
//# sourceMappingURL=diagnostics.test.js.map