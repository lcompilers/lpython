This provides a PoC for using VSCode’s official language server module, which replaces the existing LSP’s JSONRPC and LPythonServer code base. Time spent on it was half a day, but the bugs it has fixed save us more than a week of implementing threading with the previous implementation. Here is the gist of what it does and what it does not: (note that this implementation only adds diagnostics to the extension, symbol look up will be next)

## What does it add?

- The Language Server is now written in TypeScript, which uses [Microsoft’s official language server module](https://github.com/microsoft/vscode-languageserver-node).
- This means that the JSON RPC module earlier written is now replaced with the imported JSON RPC Client from the module.
- This also means that the code which “communicated” with the LPython compiler to get diagnostics is now reduced to a single compiler execution with a custom flag.
- This PR adds a flag `--show-errors` which prints the diagnostics from the file to the console. This is used by the VSCode Language Server (written in TS) which catches the stdout and parses into JSON, and this is literally 5-6 lines of code.
- This PR also fixes many bugs existing in the previous implementation of LSP, which include: 1. Absence of asynchronous launch to the compiler, and 2. Frequent bugs of the extension crashing while trying to show diagnostics to the VSCode window.
- This PR also makes sure that the extension updates the diagnostics shown on the VSCode window after each edit, without needing to save the document explicitly. This improves the user experience.

## What does it not do?

- It does not replace [MessageHandler](https://github.com/lcompilers/lpython/blob/main/src/libasr/lsp/MessageHandler.cpp) code written previously to fetch diagnostics and is instead used by the flag `--show-errors` to get diagnostics -> convert to RapidJSON object -> convert the object to a string -> print (`cout`) to the console.

The goal of this PR is to show the demo of how it can make the experience really smooth by adding features to the VSCode extension. The steps are:

1. Add a flag to the LPython compiler if it doesn’t exist for your feature yet. See: https://github.com/ankitaS11/lpython/blob/lsp/vscode-ts/src/bin/lpython.cpp#L944
2. On running the lpython compiler command with the given flag, it should print out the information to stdout. See: https://github.com/ankitaS11/lpython/blob/lsp/vscode-ts/src/bin/lpython.cpp#L265
3. The extension will catch the printed information into stdout, and parse it into a JSON object. See: https://github.com/ankitaS11/lpython/blob/lsp/vscode-ts/editor/vscode/lsp-sample/server/src/server.ts#L246 and https://github.com/ankitaS11/lpython/blob/lsp/vscode-ts/editor/vscode/lsp-sample/server/src/server.ts#L192
4. The JSON object will be used to convert it into a relevant diagnostic object, which is then returned. See: https://github.com/ankitaS11/lpython/blob/lsp/vscode-ts/editor/vscode/lsp-sample/server/src/server.ts#L252

That’s it. :)

This reduces the efforts for anyone trying to implement features to just C++ (in LPython Compiler code-base), and for those trying to merge those features into LSP can just parse the information into JSON in the extension, and let the extension handle the rest.

In order to use it:

1. Clone https://github.com/ankitaS11/lpython and check out to `lsp/vscode-ts` branch.
2. Go to: `editor/vscode/lsp-sample/server/src/server.ts` file and replace the binary path of LPython in line number 201 to your binary path. For example, see: https://github.com/ankitaS11/lpython/blob/lsp/vscode-ts/editor/vscode/lsp-sample/server/src/server.ts#L201

Compile lpython with LSP flag on:

```bash
conda activate lp # or use your environment name here
cmake -DCMAKE_BUILD_TYPE=Debug -DWITH_LLVM=yes -DWITH_STACKTRACE=yes -DWITH_LFORTRAN_BINARY_MODFILES=no -DWITH_LSP=yes .
cmake --build . -j16
```

Now build the extension:

```bash
cd editor/vscode/lsp-sample/ && npm install && npm run compile
```

Now open VSCode in the extension folder (`code editor/vscode/lsp-sample/`) and run `ctrl + shift + D`, click on “Run and Debug” and choose VSCode Extension Development, and test the extension. :)

In case you want to package the extension, you can do:

```bash
npm install -g vsce
vsce package
```

This will generate a .vsix file in your `editor/vscode/lsp-sample` folder, which can then be imported as an extension. You can go to extensions in VSCode, click on the three dots on the top right, click on “Install from VSIX” and select the VSIX, and done (may require a reload). The extension has now been installed.

Please feel free to question/comment/suggest or give feedback on any of this work, as I look forward to implementing document symbol lookup if everything looks good as it is. The extension, as far as diagnostics are concerned, is fully fledged ready to be tried by any user.
