# Contributing

Thanks for your interest in LPython!

We welcome contributions from anyone, even if you are new to compilers or open source in general.
It might sound daunting at first to contribute to a compiler, but do not worry, it is not that complicated.
We will help you with any technical issues you face and provide support so your contribution gets merged.

Code is not all we have though. You can help us write documentations and tutorials for other contributors.

Let's get started!

### Basic Setup

Make sure you have set up LPython for development on your local machine and can do a successful build. If not, follow the instructions provided in the [installation-docs](./installation.md) for getting started.

### Creating a contribution

All contributions to a public repository on GitHub require the contributor to open a Pull Request (PR) against the repository. Follow the steps below to open a new PR.

- Create a new branch
    ```bash
    git checkout -b [branch-name]
    ```
    Replace `[branch-name]` with a suitable name relevant to the changes you desire to make. For example, if you plan to fix an issue, you can name your branch `fixes-issue25`.


- Make changes in the relevant file(s).
  Test the changes you make. Try building the project. Run some tests to verify your code does not break anything.

  Check the [installation-docs](./installation.md) for more details on building and testing.

- Once you are assured, go ahead and push a commit with the changed files.
    ```bash
    git add FILE1 (FILE2 ...)
    git commit -m "YOUR_COMMIT_MESSAGE"
    ```
    Replace `FILE1 (FILE2 ...)` with the file names (with file extension) of changed files.

    Write a short but descriptive commit message. [Here](https://chris.beams.io/posts/git-commit/) are some great tips on writing good commit messages.

- Open a Pull Request (PR)
    - Go to your project fork on GitHub.
    - Select the branch which contains your commit.
      ![image](https://github.com/kmr-srbh/lpython/assets/151380951/fb595307-9610-44f3-89d0-0079a90fcf9e)
    - Above the list of files, select the green button which says <kbd>Compare & pull request</kbd>. If you do not see this option, select the <kbd>Contribute</kbd> option.
      ![Screenshot from 2024-02-17 11-52-41](https://github.com/kmr-srbh/lpython/assets/151380951/08b3dc25-f5d1-4624-9887-2ede565230bc)
    - Type a descriptive title. Provide further details in the Description section below. If your PR fixes an issue, link it with the issue by appending `fixes #[issue-number]` to your PR title.
      ![image](https://github.com/kmr-srbh/lpython/assets/151380951/3f2245df-42f4-44e5-9c20-d6d7789ac894)
    - Once you are done, select the <kbd>Create Pull Request</kbd> button to open a new PR.
      
  Congratulations! You just made your first contribution! 🎉
  
### What next?
Sit back and relax or go on a bug hunt.

Reviewing a PR may take some time. We request you to patiently wait for any feedback or action on your PR. If you are asked to make changes, push commits to the same branch to automatically include them in the open PR.

# Add New Features

LPython shares it's architectural design with LFortran. The example below shows the steps it would take to create a caret binary operator (**^**) in LFortran. The caret binary operator computes the average value of the two operands.

 - ### Create New Token(s)

    We extend the *tokenizer.re* as well as *parser.yy* to add the new token **^**.
    We also tell LFortran how to print the new token in *parser.cpp*.
    
    *src/lfortran/parser/tokenizer.re*
    ```
    // "^" { RET(TK_CARET) }
    ```
    
    *src/lfortran/parser/parser.yy*
    ```
    %token TK_CARET "^"
    ```
    
    *src/lfortran/parser/parser.cpp*
    ```
    std:string token2text(const int token)
    {
        switch (token) {
            T(TK_CARET, "^")
        }
    }
    ```
    The added code is tested with `lfortran --show-tokens examples2/expr2.f90`

- ### Parse the New Token

    Now we have to parse the new operator. We add it to the AST by extending the BinOp with a caret operator and modifying the *AST.asdl* file. Then we add it in *parse.yy* to properly parse and generate the new AST in *semantics.h*.Finally we extend *pickle.cpp* so that the new operator can print itself.
    
    
    
    *grammar/AST.asdl*
    ```
    operator = Add | Sub | Mul | Div | Pow | Caret
    ``` 
    
    *src/lfortran/parser/parser.yy*
    ```
    %left "^"
    expr
        : id { $$=$1; }
        | expr "^" expr { $$ = CARET($1, $3, @$); }
    ```
    
    *src/lfortran/parser/semantics.h*
    ```
    #define CARET(x,y,l) make_BinOp_t(p.m_a, l, EXPR(x), operatorType::Caret, EXPR(y))
    ```
    
    *src/lfortran/pickle.cpp*
    ```
    std::string op2str(const operatorType type)
    {
        switch (type) {
            case (operatorType::Caret) : return "^";
        } // now the caret operator can print itself
       
    }
    ```
    The section is tested with `lfortran --show-ast examples/expr2.f90`

- ### Implement the Semantics of the New Token

    We first extend the ASR in *ASR.asdl* and add ^ as a BinOp operator option. 
    
    *src/libasr/ASR.asdl*
    ```
    binop = Add | Sub | Mul | Div | Pow | Caret
    ```
    
    *src/lfortran/semantics/ast_common_visitor.h*
    ```
    namespace LFortran {
    class CommonVisitorMethods {
    public:
      inline static void visit_BinOp(Allocator &al, const AST::BinOp_t &x,
                                     ASR::expr_t *&left, ASR::expr_t *&right,
                                     ASR::asr_t *&asr) {
        ASR::binopType op;
        switch (x.m_op) {
        
        case (AST::Caret):
          op = ASR::Caret;
          break;
        }
        if (LFortran::ASRUtils::expr_value(left) != nullptr &&
            LFortran::ASRUtils::expr_value(right) != nullptr) {
          if (ASR::is_a<LFortran::ASR::Integer_t>(*dest_type)) {
            int64_t left_value = ASR::down_cast<ASR::ConstantInteger_t>(
                                     LFortran::ASRUtils::expr_value(left))
                                     ->m_n;
            int64_t right_value = ASR::down_cast<ASR::ConstantInteger_t>(
                                      LFortran::ASRUtils::expr_value(right))
                                      ->m_n;
            int64_t result;
            switch (op) {
            case (ASR::Caret):
              result = (left_value + right_value)/2;
              break;
            }
      }
    }
    }
    }
    }
    ```
    
    Then we transform it from AST to ASR by extending *src/lfortran/semantics/ast_common_visitor.h*. 
    
    We also add it into compile time evaluation triggered by expressions such as `e = (2+3)^5` which is evaluated at compile time. An expression such as `e = x^5` is evaluated at run time only.
    
    The section is tested with `lfortran --show-asr examples/expr2.f90`
    ### Implement the New Token in LLVM
    
    To implement in LLVM, we extend the BinOp translation by handling the new operator. We first add the two numbers then divide by two.
  
    *src/lfortran/codegen/asr_to_llvm.cpp*
    ```
        void visit_BinOp(const ASR::BinOp_t &x) {
            if (x.m_value) {
                this->visit_expr_wrapper(x.m_value, true);
                return;
            }
            this->visit_expr_wrapper(x.m_left, true);
            llvm::Value *left_val = tmp;
            this->visit_expr_wrapper(x.m_right, true);
            llvm::Value *right_val = tmp;
            if (x.m_type->type == ASR::ttypeType::Integer || 
                x.m_type->type == ASR::ttypeType::IntegerPointer) {
                switch (x.m_op) {
                    
                    case ASR::binopType::Caret: {
                        tmp = builder->CreateAdd(left_val, right_val);
                        llvm::Value *two = llvm::ConstantInt::get(context,
                            llvm::APInt(32, 2, true));
                        tmp = builder->CreateUDiv(tmp, two);
                        break;
                    };
    }
    }
    }
    ```
    
    The section is tested with `lfortran --show-llvm examples/expr2.f90`
    
    Now when LLVM works, we can test the final executable by:
    ```
    lfortran examples/expr2.f90
    ./a.out
    ```
    And it should print 6.
    
    It also works interactively:
    ```
    $ lfortran
    Interactive Fortran. Experimental prototype, not ready for end users.
      * Use Ctrl-D to exit
      * Use Enter to submit
      * Use Alt-Enter or Ctrl-N to make a new line
        - Editing (Keys: Left, Right, Home, End, Backspace, Delete)
        - History (Keys: Up, Down)
    >>> 4^8                                                                  1,4   ]
    6
    >>> integer :: x                                                         1,13  ]
    >>> x = 4                                                                1,6   ]
    >>> x^8                                                                  1,4   ]
    6
    ```
    
## Reach Out 

If you have any questions or need help, please ask as on our [mailinglist](https://groups.io/g/lfortran) or headover to [Zulip Chat](https://lfortran.zulipchat.com/).

Please note that all participants of this project are expected to follow our Code of Conduct. By participating in this project you agree to abide by its terms. See [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

By submitting a PR you agree to license your contribution under LPython's BSD [license](LICENSE) unless explicitly noted otherwise.
