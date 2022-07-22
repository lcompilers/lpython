"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.run = void 0;
/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
const path = require("path");
const Mocha = require("mocha");
const glob = require("glob");
function run() {
    // Create the mocha test
    const mocha = new Mocha({
        ui: 'tdd',
        color: true
    });
    mocha.timeout(100000);
    const testsRoot = __dirname;
    return new Promise((resolve, reject) => {
        glob('**.test.js', { cwd: testsRoot }, (err, files) => {
            if (err) {
                return reject(err);
            }
            // Add files to the test suite
            files.forEach(f => mocha.addFile(path.resolve(testsRoot, f)));
            try {
                // Run the mocha test
                mocha.run(failures => {
                    if (failures > 0) {
                        reject(new Error(`${failures} tests failed.`));
                    }
                    else {
                        resolve();
                    }
                });
            }
            catch (err) {
                console.error(err);
                reject(err);
            }
        });
    });
}
exports.run = run;
//# sourceMappingURL=index.js.map