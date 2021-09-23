# Contributing

We welcome contributions from anyone, even if you are new to open source. It
might sound daunting to contribute to a compiler at first, but please do, it is
not complicated. We will help you with any technical issues and help improve
your contribution so that it can be merged.

## Basic Setup

To contribute, make sure your set up:

* Your username + email
* Your `~/.gitconfig`
* Your shell prompt to display the current branch name

### Fork LFortran

Step 1. Create a fork of the [project repository](https://gitlab.com/lfortran/lfortran)

Step 2. Set up your [SSH key](https://docs.gitlab.com/ee/ssh/) with GitLab

Step 3. Clone the project repository from GitLab and set up your remote repository

```
git clone https://gitlab.com/lfortran/lfortran.git
cd lfortran
git remote add REMOTE_NAME git@gitlab.com:YOUR_GITLAB_ID/lfortran.git
```
:fontawesome-solid-edit: `REMOTE_NAME` is the name of your remote repository and could be any name you like, for example your first name.

:fontawesome-solid-edit: `YOUR_GITLAB_ID` is your user ID on GitLab and should be part of your account path.

You can use `git remote -v` to check if the new remote is set up correctly.

### Send a New Merge Request

Step 1. Create a new branch
```
git checkout -b fix1
```

Step 2. Make changes in relevant file(s)

Step 3. Commit the changes:

```
git add FILE1 (FILE2 ...)
git commit -m "YOUR_COMMIT_MESSAGE"
```

[Here](https://chris.beams.io/posts/git-commit/) are some great tips on writing good commit messages.

Step 4. Check to ensure that your changes look good

```
git log --pretty=oneline --graph --decorate --all
```
Step 5. Send the merge request

```
git push REMOTE_NAME fix1
```

The command will push the new branch `fix1` into your remote repository `REMOTE_NAME` that you created earlier. Additionally, it will also display a link that you can click on to open the new merge request. After clicking on the link, write a title and a concise description then click the "Create" button. Yay you are now all set.

## Add New Features

The example below shows the steps it would take to create a caret binary operator **^** which computes the average value of the two operands.

### Create New Token(s)

We extend the *tokenizer.re* as well as *parser.yy* to add the new token **^**.
We also tell LFortran how to print the new token in *parser.cpp*.

:fontawesome-solid-code: *src/lfortran/parser/tokenizer.re*
```
// "^" { RET(TK_CARET) }
```

:fontawesome-solid-code: *src/lfortran/parser/parser.yy*
```
%token TK_CARET "^"
```

:fontawesome-solid-code: *src/lfortran/parser/parser.cpp*
```
std:string token2text(const int token)
{
    switch (token) {
        T(TK_CARET, "^")
    }
}
```
The added code is tested with `lfortran --show-tokens examples2/expr2.f90`

### Parse the New Token
 
Now we have to parse the new operator. We add it to the AST by extending the BinOp with a caret operator and modifying the *AST.asdl* file. Then we add it in *parse.yy* to properly parse and generate the new AST in *semantics.h*.Finally we extend *pickle.cpp* so that the new operator can print itself.



:fontawesome-solid-code:*grammar/AST.asdl*
```
operator = Add | Sub | Mul | Div | Pow | Caret
``` 

:fontawesome-solid-code:*src/lfortran/parser/parser.yy*
```
%left "^"

expr
    : id { $$=$1; }
    | expr "^" expr { $$ = CARET($1, $3, @$); }
```

:fontawesome-solid-code:*src/lfortran/parser/semantics.h*
```
#define CARET(x,y,l) make_BinOp_t(p.m_a, l, EXPR(x), operatorType::Caret, EXPR(y))
```

:fontawesome-solid-code:*src/lfortran/pickle.cpp*

```
std::string op2str(const operatorType type)
{
    switch (type) {
        case (operatorType::Caret) : return "^";
    } // now the caret operator can print itself
   
}
```
The section is tested with `lfortran --show-ast examples/expr2.f90`

### Implement the Semantics of the New Token

We first extend the ASR in *ASR.asdl* and add ^ as a BinOp operator option. 

:fontawesome-solid-code:*grammar/ASR.asdl*
```
binop = Add | Sub | Mul | Div | Pow | Caret
```
:fontawesome-solid-code:*src/lfortran/semantics/ast_common_visitor.h*
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

To implement in LLVM, we extend the BinOp
translation by handling the new operator. We first add the two numbers then
divide by two.
:fontawesome-solid-code:*src/lfortran/codegen/asr_to_llvm.cpp*
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

If you have any questions or need help, please ask as at our
[mailinglist](https://groups.io/g/lfortran) or a
[chat](https://lfortran.zulipchat.com/).

Please note that all participants of this project are expected to follow our
Code of Conduct. By participating in this project you agree to abide by its
terms. See [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

By submitting an MR you agree to license your contribution under
the LFortran's BSD [license](LICENSE) unless explicitly noted otherwise.
