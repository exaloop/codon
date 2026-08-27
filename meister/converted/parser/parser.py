import itertools

from ...bridge import *
from ..ast import nodes as ast
from .pegen import *


# Keywords and soft keywords are listed at the end of the parser definition.
class CodonParser(Parser):
    _: bool  # Codon auto-deduce hack

    def start(self) -> Optional[ast.SuiteStmt]:
        # start: file
        mark = self._mark()
        if file := self.file():
            file = Codon.unwrap(file)
            return file
        self._reset(mark)
        file = None
        return None

    def file(self) -> Optional[ast.SuiteStmt]:
        # file: statements? $
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.statements(),) and (self.expect_type(tokenize.Tokens.ENDMARKER)):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.suite(
                a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def interactive(self) -> Optional[ast.SuiteStmt]:
        # interactive: statement_newline
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if a := self.statement_newline():
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.suite(
                a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def fstring(self) -> Optional[ast.StringExpr]:
        # fstring: FSTRING_START fstring_mid* FSTRING_END
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (self.fstring_start()) and (b := self._loop0_1(),) and (self.fstring_end()):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.StringExpr(
                strings=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        b = None
        return None

    def statements(self) -> Optional[List[ast.Stmt]]:
        # statements: statement+
        mark = self._mark()
        if a := self._loop1_2():
            a = Codon.unwrap(a)
            return list(itertools.chain.from_iterable(a))
        self._reset(mark)
        a = None
        return None

    def statement(self) -> Optional[List[ast.Stmt]]:
        # statement: compound_stmt | simple_stmts
        mark = self._mark()
        if a := self.compound_stmt():
            a = Codon.unwrap(a)
            return [a]
        self._reset(mark)
        a = None
        if a := self.simple_stmts():
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def statement_newline(self) -> Optional[List[ast.Stmt]]:
        # statement_newline: compound_stmt NEWLINE | simple_stmts | NEWLINE | $
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.compound_stmt()) and (self.expect_type(tokenize.Tokens.NEWLINE)):
            a = Codon.unwrap(a)
            return [a]
        self._reset(mark)
        a = None
        if simple_stmts := self.simple_stmts():
            simple_stmts = Codon.unwrap(simple_stmts)
            return simple_stmts
        self._reset(mark)
        simple_stmts = None
        if self.expect_type(tokenize.Tokens.NEWLINE):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return [
                ast.SuiteStmt(
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                )
            ]
        self._reset(mark)
        if self.expect_type(tokenize.Tokens.ENDMARKER):
            return None
        self._reset(mark)
        return None

    def simple_stmts(self) -> Optional[List[ast.Stmt]]:
        # simple_stmts: simple_stmt !';' NEWLINE | ';'.simple_stmt+ ';'? NEWLINE
        mark = self._mark()
        if (
            (a := self.simple_stmt())
            and (self.negative_lookahead(self.expect_literal, ";"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
        ):
            a = Codon.unwrap(a)
            return [a]
        self._reset(mark)
        a = None
        if (
            (a := self._gather_3())
            and (self.expect_literal(";"),)
            and (self.expect_type(tokenize.Tokens.NEWLINE))
        ):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    @memoize
    def simple_stmt(self) -> Optional[ast.Stmt]:
        # simple_stmt: assignment | &("print" !'(') print_stmt | star_expressions | &'return' return_stmt | &('import' | 'from') import_stmt | &'raise' raise_stmt | 'pass' | &'del' del_stmt | &'yield' yield_stmt | &'assert' assert_stmt | 'break' | 'continue' | &'global' global_stmt | &'nonlocal' nonlocal_stmt
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if assignment := self.assignment():
            assignment = Codon.unwrap(assignment)
            return assignment
        self._reset(mark)
        assignment = None
        if (
            self.positive_lookahead(
                self._tmp_5,
            )
        ) and (print_stmt := self.print_stmt()):
            print_stmt = Codon.unwrap(print_stmt)
            return print_stmt
        self._reset(mark)
        print_stmt = None
        if e := self.star_expressions():
            e = Codon.unwrap(e)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.ExprStmt(
                expr=e,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        e = None
        if (self.positive_lookahead(self.expect_literal, "return")) and (
            return_stmt := self.return_stmt()
        ):
            return_stmt = Codon.unwrap(return_stmt)
            return return_stmt
        self._reset(mark)
        return_stmt = None
        if (
            self.positive_lookahead(
                self._tmp_6,
            )
        ) and (import_stmt := self.import_stmt()):
            import_stmt = Codon.unwrap(import_stmt)
            return import_stmt
        self._reset(mark)
        import_stmt = None
        if (self.positive_lookahead(self.expect_literal, "raise")) and (
            raise_stmt := self.raise_stmt()
        ):
            raise_stmt = Codon.unwrap(raise_stmt)
            return raise_stmt
        self._reset(mark)
        raise_stmt = None
        if self.expect_literal("pass"):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.SuiteStmt(
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        if (self.positive_lookahead(self.expect_literal, "del")) and (del_stmt := self.del_stmt()):
            del_stmt = Codon.unwrap(del_stmt)
            return del_stmt
        self._reset(mark)
        del_stmt = None
        if (self.positive_lookahead(self.expect_literal, "yield")) and (
            yield_stmt := self.yield_stmt()
        ):
            yield_stmt = Codon.unwrap(yield_stmt)
            return yield_stmt
        self._reset(mark)
        yield_stmt = None
        if (self.positive_lookahead(self.expect_literal, "assert")) and (
            assert_stmt := self.assert_stmt()
        ):
            assert_stmt = Codon.unwrap(assert_stmt)
            return assert_stmt
        self._reset(mark)
        assert_stmt = None
        if self.expect_literal("break"):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BreakStmt(
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        if self.expect_literal("continue"):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.ContinueStmt(
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        if (self.positive_lookahead(self.expect_literal, "global")) and (
            global_stmt := self.global_stmt()
        ):
            global_stmt = Codon.unwrap(global_stmt)
            return global_stmt
        self._reset(mark)
        global_stmt = None
        if (self.positive_lookahead(self.expect_literal, "nonlocal")) and (
            nonlocal_stmt := self.nonlocal_stmt()
        ):
            nonlocal_stmt = Codon.unwrap(nonlocal_stmt)
            return nonlocal_stmt
        self._reset(mark)
        nonlocal_stmt = None
        return None

    def compound_stmt(self) -> Optional[ast.Stmt]:
        # compound_stmt: &('def' | '@' | 'async') function_def | &'if' if_stmt | &('class' | '@') class_def | &('with' | 'async') with_stmt | &('for' | '@' | 'async') for_stmt | &'try' try_stmt | &'while' while_stmt | match_stmt | custom_stmt
        mark = self._mark()
        if (
            self.positive_lookahead(
                self._tmp_7,
            )
        ) and (function_def := self.function_def()):
            function_def = Codon.unwrap(function_def)
            return function_def
        self._reset(mark)
        function_def = None
        if (self.positive_lookahead(self.expect_literal, "if")) and (if_stmt := self.if_stmt()):
            if_stmt = Codon.unwrap(if_stmt)
            return if_stmt
        self._reset(mark)
        if_stmt = None
        if (
            self.positive_lookahead(
                self._tmp_8,
            )
        ) and (class_def := self.class_def()):
            class_def = Codon.unwrap(class_def)
            return class_def
        self._reset(mark)
        class_def = None
        if (
            self.positive_lookahead(
                self._tmp_9,
            )
        ) and (with_stmt := self.with_stmt()):
            with_stmt = Codon.unwrap(with_stmt)
            return with_stmt
        self._reset(mark)
        with_stmt = None
        if (
            self.positive_lookahead(
                self._tmp_10,
            )
        ) and (for_stmt := self.for_stmt()):
            for_stmt = Codon.unwrap(for_stmt)
            return for_stmt
        self._reset(mark)
        for_stmt = None
        if (self.positive_lookahead(self.expect_literal, "try")) and (try_stmt := self.try_stmt()):
            try_stmt = Codon.unwrap(try_stmt)
            return try_stmt
        self._reset(mark)
        try_stmt = None
        if (self.positive_lookahead(self.expect_literal, "while")) and (
            while_stmt := self.while_stmt()
        ):
            while_stmt = Codon.unwrap(while_stmt)
            return while_stmt
        self._reset(mark)
        while_stmt = None
        if match_stmt := self.match_stmt():
            match_stmt = Codon.unwrap(match_stmt)
            return match_stmt
        self._reset(mark)
        match_stmt = None
        if custom_stmt := self.custom_stmt():
            custom_stmt = Codon.unwrap(custom_stmt)
            return custom_stmt
        self._reset(mark)
        custom_stmt = None
        return None

    def assignment(self) -> Optional[ast.Stmt]:
        # assignment: NAME ':' expression ['=' annotated_rhs] | ('(' single_target ')' | single_subscript_attribute_target) ':' expression ['=' annotated_rhs] | ((star_targets '='))+ (yield_expr | star_expressions) !'=' | single_target augassign ~ (yield_expr | star_expressions) | invalid_assignment
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (a := self.name())
            and (self.expect_literal(":"))
            and (b := self.expression())
            and (c := self._tmp_11(),)
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.AssignStmt(
                lhs=ast.IdExpr(
                    value=a.string,
                    lineno=a.start[0],
                    col_offset=a.start[1],
                    end_lineno=a.end[0],
                    end_col_offset=a.end[1],
                ),
                rhs=c,
                type_expr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        if (
            (a := self._tmp_12())
            and (self.expect_literal(":"))
            and (b := self.expression())
            and (c := self._tmp_13(),)
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.AssignStmt(
                lhs=a,
                rhs=c,
                type_expr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        if (
            (a := self._loop1_14())
            and (b := self._tmp_15())
            and (self.negative_lookahead(self.expect_literal, "="))
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.assignments(
                a,
                b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        cut = False
        if (
            (a := self.single_target())
            and (b := self.augassign())
            and (cut := True)
            and (c := self._tmp_16())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            c = Codon.unwrap(c)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.AssignStmt(
                lhs=a,
                rhs=ast.BinaryExpr(
                    lexpr=a.clone(),
                    op=b,
                    rexpr=c,
                    in_place=True,
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                ),
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        if cut:
            return None
        a = None
        b = None
        c = None
        if self.call_invalid_rules and (self.invalid_assignment()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def annotated_rhs(self) -> Optional[ast.Expr]:
        # annotated_rhs: yield_expr | star_expressions
        mark = self._mark()
        if yield_expr := self.yield_expr():
            yield_expr = Codon.unwrap(yield_expr)
            return yield_expr
        self._reset(mark)
        yield_expr = None
        if star_expressions := self.star_expressions():
            star_expressions = Codon.unwrap(star_expressions)
            return star_expressions
        self._reset(mark)
        star_expressions = None
        return None

    def augassign(self) -> Optional[str]:
        # augassign: '+=' | '-=' | '*=' | '@=' | '/=' | '%=' | '&=' | '|=' | '^=' | '<<=' | '>>=' | '**=' | '//='
        mark = self._mark()
        if self.expect_literal("+="):
            return "+"
        self._reset(mark)
        if self.expect_literal("-="):
            return "-"
        self._reset(mark)
        if self.expect_literal("*="):
            return "*"
        self._reset(mark)
        if self.expect_literal("@="):
            return "@"
        self._reset(mark)
        if self.expect_literal("/="):
            return "/"
        self._reset(mark)
        if self.expect_literal("%="):
            return "%"
        self._reset(mark)
        if self.expect_literal("&="):
            return "&"
        self._reset(mark)
        if self.expect_literal("|="):
            return "|"
        self._reset(mark)
        if self.expect_literal("^="):
            return "^"
        self._reset(mark)
        if self.expect_literal("<<="):
            return "<<"
        self._reset(mark)
        if self.expect_literal(">>="):
            return ">>"
        self._reset(mark)
        if self.expect_literal("**="):
            return "**"
        self._reset(mark)
        if self.expect_literal("//="):
            return "//"
        self._reset(mark)
        return None

    def return_stmt(self) -> Optional[ast.ReturnStmt]:
        # return_stmt: 'return' star_expressions?
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (self.expect_literal("return")) and (a := self.star_expressions(),):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.ReturnStmt(
                expr=a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def raise_stmt(self) -> Optional[ast.ThrowStmt]:
        # raise_stmt: 'raise' expression ['from' expression] | 'raise'
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (self.expect_literal("raise")) and (a := self.expression()) and (b := self._tmp_17(),):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.ThrowStmt(
                expr=a,
                from_expr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if self.expect_literal("raise"):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.ThrowStmt(
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        return None

    def global_stmt(self) -> Optional[ast.SuiteStmt]:
        # global_stmt: 'global' ','.NAME+
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (self.expect_literal("global")) and (a := self._gather_18()):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.globals(
                a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def nonlocal_stmt(self) -> Optional[ast.SuiteStmt]:
        # nonlocal_stmt: 'nonlocal' ','.NAME+
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (self.expect_literal("nonlocal")) and (a := self._gather_20()):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.globals(
                a,
                non_local=True,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def print_stmt(self) -> Optional[ast.PrintStmt]:
        # print_stmt: "print" ','.expression+ ','?
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.expect_literal("print"))
            and (a := self._gather_22())
            and (b := self.expect_literal(","),)
        ):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.PrintStmt(
                items=a,
                no_newline=bool(b),
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        return None

    def del_stmt(self) -> Optional[ast.SuiteStmt]:
        # del_stmt: 'del' del_targets &(';' | NEWLINE) | invalid_del_stmt
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.expect_literal("del"))
            and (a := self.del_targets())
            and (
                self.positive_lookahead(
                    self._tmp_24,
                )
            )
        ):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.deletes(
                a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if self.call_invalid_rules and (self.invalid_del_stmt()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def yield_stmt(self) -> Optional[ast.Stmt]:
        # yield_stmt: 'yield' 'from' expression | 'yield' star_expressions?
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.expect_literal("yield"))
            and (self.expect_literal("from"))
            and (a := self.expression())
        ):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.YieldFromStmt(
                expr=a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if (self.expect_literal("yield")) and (a := self.star_expressions(),):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.YieldStmt(
                expr=a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def assert_stmt(self) -> Optional[ast.AssertStmt]:
        # assert_stmt: 'assert' expression [',' expression]
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (self.expect_literal("assert")) and (a := self.expression()) and (b := self._tmp_25(),):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.AssertStmt(
                expr=a,
                message=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        return None

    @memoize
    def import_stmt(self) -> Optional[ast.Stmt]:
        # import_stmt: invalid_import | import_name | import_from
        mark = self._mark()
        if self.call_invalid_rules and (self.invalid_import()):
            return None  # pragma: no cover
        self._reset(mark)
        if import_name := self.import_name():
            import_name = Codon.unwrap(import_name)
            return import_name
        self._reset(mark)
        import_name = None
        if import_from := self.import_from():
            import_from = Codon.unwrap(import_from)
            return import_from
        self._reset(mark)
        import_from = None
        return None

    def import_name(self) -> Optional[ast.SuiteStmt]:
        # import_name: 'import' dotted_as_names
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (self.expect_literal("import")) and (a := self.dotted_as_names()):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.imports(
                a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def import_from(self) -> Optional[ast.SuiteStmt]:
        # import_from: 'from' (('.' | '...'))* dotted_name 'import' import_from_targets | 'from' (('.' | '...'))+ 'import' import_from_targets
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.expect_literal("from"))
            and (a := self._loop0_26(),)
            and (b := self.dotted_name())
            and (self.expect_literal("import"))
            and (c := self.import_from_targets())
        ):
            b = Codon.unwrap(b)
            c = Codon.unwrap(c)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.imports_from(
                b,
                c,
                self.extract_import_level(a),
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        if (
            (self.expect_literal("from"))
            and (a := self._loop1_27())
            and (self.expect_literal("import"))
            and (b := self.import_from_targets())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.imports_from(
                None,
                b,
                self.extract_import_level(a),
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        return None

    def import_from_targets(self) -> Optional[List[tuple]]:
        # import_from_targets: '(' import_from_as_names ','? ')' | import_from_as_names !',' | '*' | invalid_import_from_targets
        mark = self._mark()
        if (
            (self.expect_literal("("))
            and (a := self.import_from_as_names())
            and (self.expect_literal(","),)
            and (self.expect_literal(")"))
        ):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if (import_from_as_names := self.import_from_as_names()) and (
            self.negative_lookahead(self.expect_literal, ",")
        ):
            import_from_as_names = Codon.unwrap(import_from_as_names)
            return import_from_as_names
        self._reset(mark)
        import_from_as_names = None
        if self.expect_literal("*"):
            return [("*", None, [], None, True)]
        self._reset(mark)
        if self.call_invalid_rules and (self.invalid_import_from_targets()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def import_from_as_names(self) -> Optional[List[tuple]]:
        # import_from_as_names: ','.import_from_as_name+
        mark = self._mark()
        if a := self._gather_28():
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def import_from_as_name(self) -> Optional[Tuple]:
        # import_from_as_name: dotted_name '(' import_params* ')' '->' import_param ['as' NAME] | dotted_name '(' import_params* ')' ['as' NAME] | dotted_name ':' import_param ['as' NAME] | dotted_name ['as' NAME]
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (a := self.dotted_name())
            and (self.expect_literal("("))
            and (c := self._loop0_30(),)
            and (self.expect_literal(")"))
            and (self.expect_literal("->"))
            and (d := self.import_param())
            and (b := self._tmp_31(),)
        ):
            a = Codon.unwrap(a)
            d = Codon.unwrap(d)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return (
                a,
                b,
                [
                    ast.Param(
                        type=p,
                        lineno=start_lineno,
                        col_offset=start_col_offset,
                        end_lineno=end_lineno,
                        end_col_offset=end_col_offset,
                    )
                    for p in c
                ],
                d,
                True,
            )
        self._reset(mark)
        a = None
        c = None
        d = None
        b = None
        if (
            (a := self.dotted_name())
            and (self.expect_literal("("))
            and (c := self._loop0_32(),)
            and (self.expect_literal(")"))
            and (b := self._tmp_33(),)
        ):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return (
                a,
                b,
                [
                    ast.Param(
                        type=p,
                        lineno=start_lineno,
                        col_offset=start_col_offset,
                        end_lineno=end_lineno,
                        end_col_offset=end_col_offset,
                    )
                    for p in c
                ],
                ast.IdExpr(
                    value="NoneType",
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                ),
                True,
            )
        self._reset(mark)
        a = None
        c = None
        b = None
        if (
            (a := self.dotted_name())
            and (self.expect_literal(":"))
            and (d := self.import_param())
            and (b := self._tmp_34(),)
        ):
            a = Codon.unwrap(a)
            d = Codon.unwrap(d)
            return (a, b, [], d, False)
        self._reset(mark)
        a = None
        d = None
        b = None
        if (a := self.dotted_name()) and (b := self._tmp_35(),):
            a = Codon.unwrap(a)
            return (a, b, [], None, True)
        self._reset(mark)
        a = None
        b = None
        return None

    def import_params(self) -> Optional[ast.Expr]:
        # import_params: import_param ',' | import_param &')'
        mark = self._mark()
        if (a := self.import_param()) and (self.expect_literal(",")):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if (a := self.import_param()) and (self.positive_lookahead(self.expect_literal, ")")):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def import_param(self) -> Optional[ast.Expr]:
        # import_param: expression
        mark = self._mark()
        if e := self.expression():
            e = Codon.unwrap(e)
            return e
        self._reset(mark)
        e = None
        return None

    def dotted_as_names(self) -> Optional[List[tuple]]:
        # dotted_as_names: ','.dotted_as_name+
        mark = self._mark()
        if a := self._gather_36():
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def dotted_as_name(self) -> Optional[Tuple]:
        # dotted_as_name: dotted_name ['as' NAME]
        mark = self._mark()
        if (a := self.dotted_name()) and (b := self._tmp_38(),):
            a = Codon.unwrap(a)
            return (a, b, [], None, True)
        self._reset(mark)
        a = None
        b = None
        return None

    @memoize_left_rec
    def dotted_name(self) -> Optional[str]:
        # dotted_name: dotted_name '.' NAME | NAME
        mark = self._mark()
        if (a := self.dotted_name()) and (self.expect_literal(".")) and (b := self.name()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return a + "." + b.string
        self._reset(mark)
        a = None
        b = None
        if a := self.name():
            a = Codon.unwrap(a)
            return a.string
        self._reset(mark)
        a = None
        return None

    @memoize
    def block(self) -> Optional[ast.SuiteStmt]:
        # block: NEWLINE INDENT statements DEDENT | simple_stmts | invalid_block
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.expect_type(tokenize.Tokens.INDENT))
            and (a := self.statements())
            and (self.expect_type(tokenize.Tokens.DEDENT))
        ):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.suite(
                a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if a := self.simple_stmts():
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.suite(
                a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if self.call_invalid_rules and (self.invalid_block()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def decorators(self) -> Optional[List[ast.Expr]]:
        # decorators: decorator+
        mark = self._mark()
        if _loop1_39 := self._loop1_39():
            _loop1_39 = Codon.unwrap(_loop1_39)
            return _loop1_39
        self._reset(mark)
        _loop1_39 = None
        return None

    def decorator(self) -> Optional[ast.Expr]:
        # decorator: ('@' dec_maybe_call NEWLINE) | ('@' named_expression NEWLINE)
        mark = self._mark()
        if a := self._tmp_40():
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if a := self._tmp_41():
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def dec_maybe_call(self) -> Optional[ast.Expr]:
        # dec_maybe_call: dec_primary '(' arguments? ')' | dec_primary
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (dn := self.dec_primary())
            and (self.expect_literal("("))
            and (z := self.arguments(),)
            and (self.expect_literal(")"))
        ):
            dn = Codon.unwrap(dn)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.call(
                dn,
                z[0] if z else [],
                z[1] if z else [],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        dn = None
        z = None
        if dec_primary := self.dec_primary():
            dec_primary = Codon.unwrap(dec_primary)
            return dec_primary
        self._reset(mark)
        dec_primary = None
        return None

    @memoize_left_rec
    def dec_primary(self) -> Optional[ast.Expr]:
        # dec_primary: dec_primary '.' NAME | NAME
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.dec_primary()) and (self.expect_literal(".")) and (b := self.name()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.DotExpr(
                expr=a,
                member=b.string,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if a := self.name():
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IdExpr(
                value=a.string,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def class_def(self) -> Optional[ast.ClassStmt]:
        # class_def: decorators class_def_raw | class_def_raw
        mark = self._mark()
        if (a := self.decorators()) and (b := self.class_def_raw()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.set_decorators(b, a)
        self._reset(mark)
        a = None
        b = None
        if class_def_raw := self.class_def_raw():
            class_def_raw = Codon.unwrap(class_def_raw)
            return class_def_raw
        self._reset(mark)
        class_def_raw = None
        return None

    def class_def_raw(self) -> Optional[ast.ClassStmt]:
        # class_def_raw: invalid_class_def_raw | 'class' NAME type_params? ['(' arguments? ')'] &&':' block
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if self.call_invalid_rules and (self.invalid_class_def_raw()):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expect_literal("class"))
            and (a := self.name())
            and (t := self.type_params(),)
            and (b := self._tmp_42(),)
            and (self.expect_forced(self.expect_literal(":"), "':'"))
            and (c := self.block())
        ):
            a = Codon.unwrap(a)
            c = Codon.unwrap(c)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_class(
                a.string,
                t or [],
                b,
                c,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        t = None
        b = None
        c = None
        return None

    def function_def(self) -> Optional[ast.FunctionStmt]:
        # function_def: (decorator_not_llvm*) '@' "llvm" NEWLINE (decorator*) function_def_llvm | decorators function_def_raw | function_def_raw
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (dp := self._loop0_43(),)
            and (self.expect_literal("@"))
            and (self.expect_literal("llvm"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (da := self._loop0_44(),)
            and (f := self.function_def_llvm())
        ):
            f = Codon.unwrap(f)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.set_decorators(
                f,
                (dp or [])
                + (da or [])
                + [
                    ast.IdExpr(
                        value="llvm",
                        lineno=start_lineno,
                        col_offset=start_col_offset,
                        end_lineno=end_lineno,
                        end_col_offset=end_col_offset,
                    )
                ],
            )
        self._reset(mark)
        dp = None
        da = None
        f = None
        if (d := self.decorators()) and (f := self.function_def_raw()):
            d = Codon.unwrap(d)
            f = Codon.unwrap(f)
            return self.set_decorators(f, d)
        self._reset(mark)
        d = None
        f = None
        if f := self.function_def_raw():
            f = Codon.unwrap(f)
            return f
        self._reset(mark)
        f = None
        return None

    def function_def_raw(self) -> Optional[ast.FunctionStmt]:
        # function_def_raw: invalid_def_raw | 'def' NAME type_params? &&'(' params? ')' ['->' expression] &&':' block | 'async' 'def' NAME type_params? &&'(' params? ')' ['->' expression] &&':' block
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if self.call_invalid_rules and (self.invalid_def_raw()):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expect_literal("def"))
            and (n := self.name())
            and (t := self.type_params(),)
            and (self.expect_forced(self.expect_literal("("), "'('"))
            and (params := self.params(),)
            and (self.expect_literal(")"))
            and (a := self._tmp_45(),)
            and (self.expect_forced(self.expect_literal(":"), "':'"))
            and (b := self.block())
        ):
            n = Codon.unwrap(n)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.FunctionStmt(
                name=n.string,
                items=(
                    params
                    or self.make_arguments(
                        lineno=start_lineno,
                        col_offset=start_col_offset,
                        end_lineno=end_lineno,
                        end_col_offset=end_col_offset,
                    )
                )
                + (t or []),
                ret=a,
                suite=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        n = None
        t = None
        params = None
        a = None
        b = None
        if (
            (self.expect_literal("async"))
            and (self.expect_literal("def"))
            and (n := self.name())
            and (t := self.type_params(),)
            and (self.expect_forced(self.expect_literal("("), "'('"))
            and (params := self.params(),)
            and (self.expect_literal(")"))
            and (a := self._tmp_46(),)
            and (self.expect_forced(self.expect_literal(":"), "':'"))
            and (b := self.block())
        ):
            n = Codon.unwrap(n)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.FunctionStmt(
                name=n.string,
                items=(
                    params
                    or self.make_arguments(
                        lineno=start_lineno,
                        col_offset=start_col_offset,
                        end_lineno=end_lineno,
                        end_col_offset=end_col_offset,
                    )
                )
                + (t or []),
                ret=a,
                suite=b,
                async_=True,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        n = None
        t = None
        params = None
        a = None
        b = None
        return None

    def decorator_not_llvm(self) -> Optional[ast.Expr]:
        # decorator_not_llvm: !('@' "llvm") decorator
        mark = self._mark()
        if (
            self.negative_lookahead(
                self._tmp_47,
            )
        ) and (d := self.decorator()):
            d = Codon.unwrap(d)
            return d
        self._reset(mark)
        d = None
        return None

    def function_def_llvm(self) -> Optional[ast.FunctionStmt]:
        # function_def_llvm: invalid_def_raw | 'def' NAME type_params? &&'(' params? ')' ['->' expression] &&':' llvm_block
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if self.call_invalid_rules and (self.invalid_def_raw()):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expect_literal("def"))
            and (n := self.name())
            and (t := self.type_params(),)
            and (self.expect_forced(self.expect_literal("("), "'('"))
            and (params := self.params(),)
            and (self.expect_literal(")"))
            and (a := self._tmp_48(),)
            and (self.expect_forced(self.expect_literal(":"), "':'"))
            and (b := self.llvm_block())
        ):
            n = Codon.unwrap(n)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.FunctionStmt(
                name=n.string,
                items=(
                    params
                    or self.make_arguments(
                        lineno=start_lineno,
                        col_offset=start_col_offset,
                        end_lineno=end_lineno,
                        end_col_offset=end_col_offset,
                    )
                )
                + (t or []),
                ret=a,
                suite=self.suite(
                    [
                        ast.ExprStmt(
                            expr=ast.StringExpr(
                                value=b,
                                prefix="llvm",
                                lineno=start_lineno,
                                col_offset=start_col_offset,
                                end_lineno=end_lineno,
                                end_col_offset=end_col_offset,
                            ),
                            lineno=start_lineno,
                            col_offset=start_col_offset,
                            end_lineno=end_lineno,
                            end_col_offset=end_col_offset,
                        )
                    ],
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                ),
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        n = None
        t = None
        params = None
        a = None
        b = None
        return None

    def llvm_block(self) -> Optional[str]:
        # llvm_block: NEWLINE INDENT llvm_line+ DEDENT
        mark = self._mark()
        if (
            (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.expect_type(tokenize.Tokens.INDENT))
            and (a := self._loop1_49())
            and (self.expect_type(tokenize.Tokens.DEDENT))
        ):
            a = Codon.unwrap(a)
            return "".join(a)
        self._reset(mark)
        a = None
        return None

    def llvm_line(self) -> Optional[str]:
        # llvm_line: ANY_BUT_NEWLINE* NEWLINE
        mark = self._mark()
        if (self._loop0_50(),) and (n := self.expect_type(tokenize.Tokens.NEWLINE)):
            n = Codon.unwrap(n)
            return n.line
        self._reset(mark)
        n = None
        return None

    def params(self) -> Optional[List[ast.Param]]:
        # params: invalid_parameters | parameters
        # nullable=True
        mark = self._mark()
        if self.call_invalid_rules and (self.invalid_parameters()):
            return None  # pragma: no cover
        self._reset(mark)
        if parameters := self.parameters():
            parameters = Codon.unwrap(parameters)
            return parameters
        self._reset(mark)
        parameters = None
        return None

    def parameters(self) -> Optional[List[ast.Param]]:
        # parameters: slash_no_default param_no_default* param_with_default* star_etc? | slash_with_default param_with_default* star_etc? | param_no_default+ param_with_default* star_etc? | param_with_default+ star_etc? | star_etc
        # nullable=True
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (a := self.slash_no_default())
            and (b := self._loop0_51(),)
            and (c := self._loop0_52(),)
            and (d := self.star_etc(),)
        ):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_arguments(
                pos_only=a,
                param_no_default=b,
                param_default=c,
                after_star=d,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        d = None
        if (
            (a := self.slash_with_default())
            and (b := self._loop0_53(),)
            and (c := self.star_etc(),)
        ):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_arguments(
                pos_only_with_default=a,
                param_default=b,
                after_star=c,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        if (a := self._loop1_54()) and (b := self._loop0_55(),) and (c := self.star_etc(),):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_arguments(
                param_no_default=a,
                param_default=b,
                after_star=c,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        if (a := self._loop1_56()) and (b := self.star_etc(),):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_arguments(
                param_default=a,
                after_star=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if a := self.star_etc():
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_arguments(
                after_star=a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def slash_no_default(self) -> Optional[List[Tuple[ast.Param, ast.Expr | None]]]:
        # slash_no_default: param_no_default+ '/' ',' | param_no_default+ '/' &')'
        mark = self._mark()
        if (a := self._loop1_57()) and (self.expect_literal("/")) and (self.expect_literal(",")):
            a = Codon.unwrap(a)
            return [(p, None) for p in a]
        self._reset(mark)
        a = None
        if (
            (a := self._loop1_58())
            and (self.expect_literal("/"))
            and (self.positive_lookahead(self.expect_literal, ")"))
        ):
            a = Codon.unwrap(a)
            return [(p, None) for p in a]
        self._reset(mark)
        a = None
        return None

    def slash_with_default(self) -> Optional[List[Tuple[ast.Param, ast.Expr | None]]]:
        # slash_with_default: param_no_default* param_with_default+ '/' ',' | param_no_default* param_with_default+ '/' &')'
        mark = self._mark()
        if (
            (a := self._loop0_59(),)
            and (b := self._loop1_60())
            and (self.expect_literal("/"))
            and (self.expect_literal(","))
        ):
            b = Codon.unwrap(b)
            return ([(p, None) for p in a] if a else []) + b
        self._reset(mark)
        a = None
        b = None
        if (
            (a := self._loop0_61(),)
            and (b := self._loop1_62())
            and (self.expect_literal("/"))
            and (self.positive_lookahead(self.expect_literal, ")"))
        ):
            b = Codon.unwrap(b)
            return ([(p, None) for p in a] if a else []) + b
        self._reset(mark)
        a = None
        b = None
        return None

    def star_etc(
        self,
    ) -> Optional[
        Tuple[
            ast.Param | None,
            List[Tuple[ast.Param, ast.Expr | None]],
            ast.Param | None,
            List[Tuple[ast.Param, ast.Expr | None]],
        ]
    ]:
        # star_etc: invalid_star_etc | '*' param_no_default param_maybe_default* kwds? codon_type_param* | '*' param_no_default_star_annotation param_maybe_default* kwds? codon_type_param* | '*' ',' param_maybe_default+ kwds? codon_type_param* | kwds codon_type_param* | codon_type_param*
        # nullable=True
        mark = self._mark()
        if self.call_invalid_rules and (self.invalid_star_etc()):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expect_literal("*"))
            and (a := self.param_no_default())
            and (b := self._loop0_63(),)
            and (c := self.kwds(),)
            and (t := self._loop0_64(),)
        ):
            a = Codon.unwrap(a)
            return (a, b, c, Codon.unwrap(t))
        self._reset(mark)
        a = None
        b = None
        c = None
        t = None
        if (
            (self.expect_literal("*"))
            and (a := self.param_no_default_star_annotation())
            and (b := self._loop0_65(),)
            and (c := self.kwds(),)
            and (t := self._loop0_66(),)
        ):
            a = Codon.unwrap(a)
            return (a, Codon.unwrap(b), c, Codon.unwrap(t))
        self._reset(mark)
        a = None
        b = None
        c = None
        t = None
        if (
            (self.expect_literal("*"))
            and (self.expect_literal(","))
            and (b := self._loop1_67())
            and (c := self.kwds(),)
            and (t := self._loop0_68(),)
        ):
            b = Codon.unwrap(b)
            return (None, b, c, Codon.unwrap(t))
        self._reset(mark)
        b = None
        c = None
        t = None
        if (a := self.kwds()) and (t := self._loop0_69(),):
            a = Codon.unwrap(a)
            return (None, [], a, Codon.unwrap(t))
        self._reset(mark)
        a = None
        t = None
        if (t := self._loop0_70(),):
            return (None, [], None, Codon.unwrap(t))
        self._reset(mark)
        t = None
        return None

    def kwds(self) -> Optional[ast.Param]:
        # kwds: invalid_kwds | '**' param_no_default
        mark = self._mark()
        if self.call_invalid_rules and (self.invalid_kwds()):
            return None  # pragma: no cover
        self._reset(mark)
        if (self.expect_literal("**")) and (a := self.param_no_default()):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def param_no_default(self) -> Optional[ast.Param]:
        # param_no_default: param ',' | param &')'
        mark = self._mark()
        if (a := self.param()) and (self.expect_literal(",")):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if (a := self.param()) and (self.positive_lookahead(self.expect_literal, ")")):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def param_no_default_star_annotation(self) -> Optional[ast.Param]:
        # param_no_default_star_annotation: param_star_annotation ',' | param_star_annotation &')'
        mark = self._mark()
        if (a := self.param_star_annotation()) and (self.expect_literal(",")):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if (a := self.param_star_annotation()) and (
            self.positive_lookahead(self.expect_literal, ")")
        ):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def param_with_default(self) -> Optional[Tuple[ast.Param, ast.Expr | None]]:
        # param_with_default: param default ',' | param default &')'
        mark = self._mark()
        if (a := self.param()) and (c := self.default()) and (self.expect_literal(",")):
            a = Codon.unwrap(a)
            c = Codon.unwrap(c)
            return (a, c)
        self._reset(mark)
        a = None
        c = None
        if (
            (a := self.param())
            and (c := self.default())
            and (self.positive_lookahead(self.expect_literal, ")"))
        ):
            a = Codon.unwrap(a)
            c = Codon.unwrap(c)
            return (a, c)
        self._reset(mark)
        a = None
        c = None
        return None

    def param_maybe_default(self) -> Optional[Tuple[ast.Param, ast.Expr | None]]:
        # param_maybe_default: param default? ',' | param default? &')'
        mark = self._mark()
        if (a := self.param()) and (c := self.default(),) and (self.expect_literal(",")):
            a = Codon.unwrap(a)
            return (a, c)
        self._reset(mark)
        a = None
        c = None
        if (
            (a := self.param())
            and (c := self.default(),)
            and (self.positive_lookahead(self.expect_literal, ")"))
        ):
            a = Codon.unwrap(a)
            return (a, c)
        self._reset(mark)
        a = None
        c = None
        return None

    def codon_type_param(self) -> Optional[Tuple[ast.Param, ast.Expr | None]]:
        # codon_type_param: NAME ':' type_annotation default? ',' | NAME ':' type_annotation default? &')'
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (a := self.name())
            and (self.expect_literal(":"))
            and (b := self.type_annotation())
            and (c := self.default(),)
            and (self.expect_literal(","))
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return (
                ast.Param(
                    name=a.string,
                    type=b,
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                ),
                c,
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        if (
            (a := self.name())
            and (self.expect_literal(":"))
            and (b := self.type_annotation())
            and (c := self.default(),)
            and (self.positive_lookahead(self.expect_literal, ")"))
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return (
                ast.Param(
                    name=a.string,
                    type=b,
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                ),
                c,
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        return None

    def type_annotation(self) -> Optional[ast.Expr]:
        # type_annotation: "Literal" '[' ("int" | "str" | "bool") ']' | "type"
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (a := self.expect_literal("Literal"))
            and (self.expect_literal("["))
            and (b := self._tmp_71())
            and (self.expect_literal("]"))
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IndexExpr(
                expr=ast.IdExpr(
                    value=a.string,
                    lineno=a.start[0],
                    col_offset=a.start[1],
                    end_lineno=a.end[0],
                    end_col_offset=a.end[1],
                ),
                index=ast.IdExpr(
                    value=b.string,
                    lineno=b.start[0],
                    col_offset=b.start[1],
                    end_lineno=b.end[0],
                    end_col_offset=b.end[1],
                ),
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if a := self.expect_literal("type"):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IdExpr(
                value=a.string,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def param(self) -> Optional[ast.Param]:
        # param: NAME annotation?
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.name()) and (b := self.annotation(),):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.Param(
                name=a.string,
                type=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        return None

    def param_star_annotation(self) -> Optional[ast.Param]:
        # param_star_annotation: NAME star_annotation
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.name()) and (b := self.star_annotation()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.Param(
                name=a.string,
                type=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        return None

    def annotation(self) -> Optional[ast.Expr]:
        # annotation: ':' expression
        mark = self._mark()
        if (self.expect_literal(":")) and (a := self.expression()):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def star_annotation(self) -> Optional[ast.Expr]:
        # star_annotation: ':' star_expression
        mark = self._mark()
        if (self.expect_literal(":")) and (a := self.star_expression()):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def default(self) -> Optional[ast.Expr]:
        # default: '=' expression | invalid_default
        mark = self._mark()
        if (self.expect_literal("=")) and (a := self.expression()):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if self.call_invalid_rules and (self.invalid_default()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def if_stmt(self) -> Optional[ast.IfStmt]:
        # if_stmt: invalid_if_stmt | 'if' named_expression ':' block elif_stmt | 'if' named_expression ':' block else_block?
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if self.call_invalid_rules and (self.invalid_if_stmt()):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expect_literal("if"))
            and (a := self.named_expression())
            and (self.expect_literal(":"))
            and (b := self.block())
            and (c := self.elif_stmt())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            c = Codon.unwrap(c)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IfStmt(
                cond=a,
                if_suite=b,
                else_suite=self.suite(
                    c,
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                ),
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        if (
            (self.expect_literal("if"))
            and (a := self.named_expression())
            and (self.expect_literal(":"))
            and (b := self.block())
            and (c := self.else_block(),)
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IfStmt(
                cond=a,
                if_suite=b,
                else_suite=c,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        return None

    def elif_stmt(self) -> Optional[List[ast.Stmt]]:
        # elif_stmt: invalid_elif_stmt | 'elif' named_expression ':' block elif_stmt | 'elif' named_expression ':' block else_block?
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if self.call_invalid_rules and (self.invalid_elif_stmt()):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expect_literal("elif"))
            and (a := self.named_expression())
            and (self.expect_literal(":"))
            and (b := self.block())
            and (c := self.elif_stmt())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            c = Codon.unwrap(c)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return [
                ast.IfStmt(
                    cond=a,
                    if_suite=b,
                    else_suite=self.suite(
                        c,
                        lineno=start_lineno,
                        col_offset=start_col_offset,
                        end_lineno=end_lineno,
                        end_col_offset=end_col_offset,
                    ),
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                )
            ]
        self._reset(mark)
        a = None
        b = None
        c = None
        if (
            (self.expect_literal("elif"))
            and (a := self.named_expression())
            and (self.expect_literal(":"))
            and (b := self.block())
            and (c := self.else_block(),)
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return [
                ast.IfStmt(
                    cond=a,
                    if_suite=b,
                    else_suite=c,
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                )
            ]
        self._reset(mark)
        a = None
        b = None
        c = None
        return None

    def else_block(self) -> Optional[ast.SuiteStmt]:
        # else_block: invalid_else_stmt | 'else' (('not' 'break'))* &&':' block
        mark = self._mark()
        if self.call_invalid_rules and (self.invalid_else_stmt()):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expect_literal("else"))
            and (self._loop0_72(),)
            and (self.expect_forced(self.expect_literal(":"), "':'"))
            and (b := self.block())
        ):
            b = Codon.unwrap(b)
            return b
        self._reset(mark)
        b = None
        return None

    def while_stmt(self) -> Optional[ast.WhileStmt]:
        # while_stmt: invalid_while_stmt | 'while' named_expression ':' block else_block?
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if self.call_invalid_rules and (self.invalid_while_stmt()):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expect_literal("while"))
            and (a := self.named_expression())
            and (self.expect_literal(":"))
            and (b := self.block())
            and (c := self.else_block(),)
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.WhileStmt(
                cond=a,
                suite=b,
                else_suite=c,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        return None

    def for_stmt(self) -> Optional[ast.ForStmt]:
        # for_stmt: decorators for_stmt_raw | for_stmt_raw
        mark = self._mark()
        if (a := self.decorators()) and (b := self.for_stmt_raw()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.set_decorators(b, a)
        self._reset(mark)
        a = None
        b = None
        if for_stmt_raw := self.for_stmt_raw():
            for_stmt_raw = Codon.unwrap(for_stmt_raw)
            return for_stmt_raw
        self._reset(mark)
        for_stmt_raw = None
        return None

    def for_stmt_raw(self) -> Optional[ast.ForStmt]:
        # for_stmt_raw: invalid_for_stmt | 'for' star_targets 'in' ~ star_expressions &&':' block else_block? | 'async' 'for' star_targets 'in' ~ star_expressions ':' block else_block? | invalid_for_target
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if self.call_invalid_rules and (self.invalid_for_stmt()):
            return None  # pragma: no cover
        self._reset(mark)
        cut = False
        if (
            (self.expect_literal("for"))
            and (t := self.star_targets())
            and (self.expect_literal("in"))
            and (cut := True)
            and (ex := self.star_expressions())
            and (self.expect_forced(self.expect_literal(":"), "':'"))
            and (b := self.block())
            and (el := self.else_block(),)
        ):
            t = Codon.unwrap(t)
            ex = Codon.unwrap(ex)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.ForStmt(
                var=t,
                iter=ex,
                suite=b,
                else_suite=el,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        if cut:
            return None
        t = None
        ex = None
        b = None
        el = None
        cut = False
        if (
            (self.expect_literal("async"))
            and (self.expect_literal("for"))
            and (t := self.star_targets())
            and (self.expect_literal("in"))
            and (cut := True)
            and (ex := self.star_expressions())
            and (self.expect_literal(":"))
            and (b := self.block())
            and (el := self.else_block(),)
        ):
            t = Codon.unwrap(t)
            ex = Codon.unwrap(ex)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.ForStmt(
                var=t,
                iter=ex,
                suite=b,
                else_suite=el,
                async_=True,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        if cut:
            return None
        t = None
        ex = None
        b = None
        el = None
        if self.call_invalid_rules and (self.invalid_for_target()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def with_stmt(self) -> Optional[ast.WithStmt]:
        # with_stmt: invalid_with_stmt_indent | 'with' '(' ','.with_item+ ','? ')' ':' block | 'with' ','.with_item+ ':' block | 'async' 'with' '(' ','.with_item+ ','? ')' ':' block | 'async' 'with' ','.with_item+ ':' block | invalid_with_stmt
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if self.call_invalid_rules and (self.invalid_with_stmt_indent()):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expect_literal("with"))
            and (self.expect_literal("("))
            and (a := self._gather_73())
            and (self.expect_literal(","),)
            and (self.expect_literal(")"))
            and (self.expect_literal(":"))
            and (b := self.block())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_with_stmt(
                a,
                b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (self.expect_literal("with"))
            and (a := self._gather_75())
            and (self.expect_literal(":"))
            and (b := self.block())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_with_stmt(
                a,
                b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (self.expect_literal("async"))
            and (self.expect_literal("with"))
            and (self.expect_literal("("))
            and (a := self._gather_77())
            and (self.expect_literal(","),)
            and (self.expect_literal(")"))
            and (self.expect_literal(":"))
            and (b := self.block())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_with_stmt(
                a,
                b,
                async_=True,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (self.expect_literal("async"))
            and (self.expect_literal("with"))
            and (a := self._gather_79())
            and (self.expect_literal(":"))
            and (b := self.block())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_with_stmt(
                a,
                b,
                async_=True,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if self.call_invalid_rules and (self.invalid_with_stmt()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def with_item(self) -> Optional[tuple]:
        # with_item: expression 'as' star_target &(',' | ')' | ':') | invalid_with_item | expression
        mark = self._mark()
        if (
            (e := self.expression())
            and (self.expect_literal("as"))
            and (t := self.star_target())
            and (
                self.positive_lookahead(
                    self._tmp_81,
                )
            )
        ):
            e = Codon.unwrap(e)
            t = Codon.unwrap(t)
            return (e, t)
        self._reset(mark)
        e = None
        t = None
        if self.call_invalid_rules and (self.invalid_with_item()):
            return None  # pragma: no cover
        self._reset(mark)
        if e := self.expression():
            e = Codon.unwrap(e)
            return (e, None)
        self._reset(mark)
        e = None
        return None

    def try_stmt(self) -> Optional[ast.TryStmt]:
        # try_stmt: invalid_try_stmt | 'try' &&':' block finally_block | 'try' &&':' block except_block+ else_block? finally_block?
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if self.call_invalid_rules and (self.invalid_try_stmt()):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expect_literal("try"))
            and (self.expect_forced(self.expect_literal(":"), "':'"))
            and (b := self.block())
            and (f := self.finally_block())
        ):
            b = Codon.unwrap(b)
            f = Codon.unwrap(f)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.TryStmt(
                suite=b,
                finally_suite=f,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        b = None
        f = None
        if (
            (self.expect_literal("try"))
            and (self.expect_forced(self.expect_literal(":"), "':'"))
            and (b := self.block())
            and (ex := self._loop1_82())
            and (el := self.else_block(),)
            and (f := self.finally_block(),)
        ):
            b = Codon.unwrap(b)
            ex = Codon.unwrap(ex)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.TryStmt(
                suite=b,
                items=ex,
                else_suite=el,
                finally_suite=f,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        b = None
        ex = None
        el = None
        f = None
        return None

    def except_block(self) -> Optional[ast.TryStmt.Except]:
        # except_block: invalid_except_stmt_indent | 'except' expression ['as' NAME] ':' block | 'except' ':' block | invalid_except_stmt
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if self.call_invalid_rules and (self.invalid_except_stmt_indent()):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expect_literal("except"))
            and (e := self.expression())
            and (t := self._tmp_83(),)
            and (self.expect_literal(":"))
            and (b := self.block())
        ):
            e = Codon.unwrap(e)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.TryStmt.Except(
                var=t or "",
                exc=e,
                suite=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        e = None
        t = None
        b = None
        if (self.expect_literal("except")) and (self.expect_literal(":")) and (b := self.block()):
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.TryStmt.Except(
                suite=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        b = None
        if self.call_invalid_rules and (self.invalid_except_stmt()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def except_star_block(self) -> Optional[ast.TryStmt.Except]:
        # except_star_block: invalid_except_star_stmt_indent | 'except' '*' expression ['as' NAME] ':' block | invalid_except_stmt
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if self.call_invalid_rules and (self.invalid_except_star_stmt_indent()):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expect_literal("except"))
            and (self.expect_literal("*"))
            and (e := self.expression())
            and (t := self._tmp_84(),)
            and (self.expect_literal(":"))
            and (b := self.block())
        ):
            e = Codon.unwrap(e)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.TryStmt.Except(
                var=t or "",
                exc=e,
                suite=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        e = None
        t = None
        b = None
        if self.call_invalid_rules and (self.invalid_except_stmt()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def finally_block(self) -> Optional[ast.SuiteStmt]:
        # finally_block: invalid_finally_stmt | 'finally' &&':' block
        mark = self._mark()
        if self.call_invalid_rules and (self.invalid_finally_stmt()):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expect_literal("finally"))
            and (self.expect_forced(self.expect_literal(":"), "':'"))
            and (a := self.block())
        ):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def match_stmt(self) -> Optional[ast.MatchStmt]:
        # match_stmt: "match" subject_expr ':' NEWLINE INDENT case_block+ DEDENT | invalid_match_stmt
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.expect_literal("match"))
            and (subject := self.subject_expr())
            and (self.expect_literal(":"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.expect_type(tokenize.Tokens.INDENT))
            and (cases := self._loop1_85())
            and (self.expect_type(tokenize.Tokens.DEDENT))
        ):
            subject = Codon.unwrap(subject)
            cases = Codon.unwrap(cases)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.MatchStmt(
                expr=subject,
                items=cases,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        subject = None
        cases = None
        if self.call_invalid_rules and (self.invalid_match_stmt()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def subject_expr(self) -> Optional[ast.Expr]:
        # subject_expr: star_named_expression ',' star_named_expressions? | named_expression
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (value := self.star_named_expression())
            and (self.expect_literal(","))
            and (values := self.star_named_expressions(),)
        ):
            value = Codon.unwrap(value)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.TupleExpr(
                items=[value] + (values or []),
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        value = None
        values = None
        if e := self.named_expression():
            e = Codon.unwrap(e)
            return e
        self._reset(mark)
        e = None
        return None

    def case_block(self) -> Optional[ast.MatchStmt.Case]:
        # case_block: invalid_case_block | "case" expression guard? ':' block
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if self.call_invalid_rules and (self.invalid_case_block()):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expect_literal("case"))
            and (pattern := self.expression())
            and (guard := self.guard(),)
            and (self.expect_literal(":"))
            and (body := self.block())
        ):
            pattern = Codon.unwrap(pattern)
            body = Codon.unwrap(body)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.MatchStmt.Case(
                pattern=pattern,
                guard=guard,
                suite=body,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        pattern = None
        guard = None
        body = None
        return None

    def guard(self) -> Optional[ast.Expr]:
        # guard: 'if' pipe
        mark = self._mark()
        if (self.expect_literal("if")) and (g := self.pipe()):
            g = Codon.unwrap(g)
            return g
        self._reset(mark)
        g = None
        return None

    def patterns(self) -> Optional[ast.Expr]:
        # patterns: open_sequence_pattern | pattern
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if patterns := self.open_sequence_pattern():
            patterns = Codon.unwrap(patterns)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.TupleExpr(
                items=patterns,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        patterns = None
        if pattern := self.pattern():
            pattern = Codon.unwrap(pattern)
            return pattern
        self._reset(mark)
        pattern = None
        return None

    def pattern(self) -> Optional[ast.Expr]:
        # pattern: as_pattern | or_pattern
        mark = self._mark()
        if as_pattern := self.as_pattern():
            as_pattern = Codon.unwrap(as_pattern)
            return as_pattern
        self._reset(mark)
        as_pattern = None
        if or_pattern := self.or_pattern():
            or_pattern = Codon.unwrap(or_pattern)
            return or_pattern
        self._reset(mark)
        or_pattern = None
        return None

    def as_pattern(self) -> Optional[ast.Expr]:
        # as_pattern: or_pattern 'as' pattern_capture_target | invalid_as_pattern
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (pattern := self.or_pattern())
            and (self.expect_literal("as"))
            and (target := self.pattern_capture_target())
        ):
            pattern = Codon.unwrap(pattern)
            target = Codon.unwrap(target)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BinaryExpr(
                lexpr=pattern,
                op="as",
                rexpr=ast.IdExpr(
                    value=target,
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                ),
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        pattern = None
        target = None
        if self.call_invalid_rules and (self.invalid_as_pattern()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def or_pattern(self) -> Optional[ast.Expr]:
        # or_pattern: '|'.closed_pattern+
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if patterns := self._gather_86():
            patterns = Codon.unwrap(patterns)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return (
                self.binary_chain(
                    patterns[0],
                    [("|", p) for p in patterns[1:]],
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                )
                if len(patterns) > 1
                else patterns[0]
            )
        self._reset(mark)
        patterns = None
        return None

    @memoize
    def closed_pattern(self) -> Optional[ast.Expr]:
        # closed_pattern: literal_pattern | capture_pattern | wildcard_pattern | value_pattern | group_pattern | sequence_pattern | mapping_pattern | class_pattern
        mark = self._mark()
        if literal_pattern := self.literal_pattern():
            literal_pattern = Codon.unwrap(literal_pattern)
            return literal_pattern
        self._reset(mark)
        literal_pattern = None
        if capture_pattern := self.capture_pattern():
            capture_pattern = Codon.unwrap(capture_pattern)
            return capture_pattern
        self._reset(mark)
        capture_pattern = None
        if wildcard_pattern := self.wildcard_pattern():
            wildcard_pattern = Codon.unwrap(wildcard_pattern)
            return wildcard_pattern
        self._reset(mark)
        wildcard_pattern = None
        if value_pattern := self.value_pattern():
            value_pattern = Codon.unwrap(value_pattern)
            return value_pattern
        self._reset(mark)
        value_pattern = None
        if group_pattern := self.group_pattern():
            group_pattern = Codon.unwrap(group_pattern)
            return group_pattern
        self._reset(mark)
        group_pattern = None
        if sequence_pattern := self.sequence_pattern():
            sequence_pattern = Codon.unwrap(sequence_pattern)
            return sequence_pattern
        self._reset(mark)
        sequence_pattern = None
        if mapping_pattern := self.mapping_pattern():
            mapping_pattern = Codon.unwrap(mapping_pattern)
            return mapping_pattern
        self._reset(mark)
        mapping_pattern = None
        if class_pattern := self.class_pattern():
            class_pattern = Codon.unwrap(class_pattern)
            return class_pattern
        self._reset(mark)
        class_pattern = None
        return None

    def literal_pattern(self) -> Optional[ast.Expr]:
        # literal_pattern: signed_number !('+' | '-') | complex_number | strings | 'None' | 'True' | 'False'
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (value := self.signed_number()) and (
            self.negative_lookahead(
                self._tmp_88,
            )
        ):
            value = Codon.unwrap(value)
            return value
        self._reset(mark)
        value = None
        if value := self.complex_number():
            value = Codon.unwrap(value)
            return value
        self._reset(mark)
        value = None
        if value := self.strings():
            value = Codon.unwrap(value)
            return value
        self._reset(mark)
        value = None
        if self.expect_literal("None"):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.NoneExpr(
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        if self.expect_literal("True"):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BoolExpr(
                value=True,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        if self.expect_literal("False"):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BoolExpr(
                value=False,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        return None

    def literal_expr(self) -> Optional[ast.Expr]:
        # literal_expr: signed_number !('+' | '-') | complex_number | strings | 'None' | 'True' | 'False'
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (signed_number := self.signed_number()) and (
            self.negative_lookahead(
                self._tmp_89,
            )
        ):
            signed_number = Codon.unwrap(signed_number)
            return signed_number
        self._reset(mark)
        signed_number = None
        if complex_number := self.complex_number():
            complex_number = Codon.unwrap(complex_number)
            return complex_number
        self._reset(mark)
        complex_number = None
        if strings := self.strings():
            strings = Codon.unwrap(strings)
            return strings
        self._reset(mark)
        strings = None
        if self.expect_literal("None"):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.NoneExpr(
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        if self.expect_literal("True"):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BoolExpr(
                value=True,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        if self.expect_literal("False"):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BoolExpr(
                value=False,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        return None

    def complex_number(self) -> Optional[ast.BinaryExpr]:
        # complex_number: signed_real_number '+' imaginary_number | signed_real_number '-' imaginary_number
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (real := self.signed_real_number())
            and (self.expect_literal("+"))
            and (imag := self.imaginary_number())
        ):
            real = Codon.unwrap(real)
            imag = Codon.unwrap(imag)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BinaryExpr(
                lexpr=real,
                op="+",
                rexpr=imag,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        real = None
        imag = None
        if (
            (real := self.signed_real_number())
            and (self.expect_literal("-"))
            and (imag := self.imaginary_number())
        ):
            real = Codon.unwrap(real)
            imag = Codon.unwrap(imag)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BinaryExpr(
                lexpr=real,
                op="-",
                rexpr=imag,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        real = None
        imag = None
        return None

    def signed_number(self) -> Optional[ast.Expr]:
        # signed_number: any_number | '-' any_number
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if a := self.any_number():
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if (self.expect_literal("-")) and (a := self.any_number()):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.UnaryExpr(
                op="-",
                expr=a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def signed_real_number(self) -> Optional[ast.Expr]:
        # signed_real_number: real_number | '-' real_number
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if real_number := self.real_number():
            real_number = Codon.unwrap(real_number)
            return real_number
        self._reset(mark)
        real_number = None
        if (self.expect_literal("-")) and (real := self.real_number()):
            real = Codon.unwrap(real)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.UnaryExpr(
                op="-",
                expr=real,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        real = None
        return None

    def real_number(self) -> Optional[ast.Expr]:
        # real_number: any_number
        mark = self._mark()
        if real := self.any_number():
            real = Codon.unwrap(real)
            return self.ensure_real(real)
        self._reset(mark)
        real = None
        return None

    def imaginary_number(self) -> Optional[ast.Expr]:
        # imaginary_number: any_number
        mark = self._mark()
        if imag := self.any_number():
            imag = Codon.unwrap(imag)
            return self.ensure_imaginary(imag)
        self._reset(mark)
        imag = None
        return None

    def any_number(self) -> Optional[ast.Expr]:
        # any_number: NUMBER NUMBER_SUFFIX?
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (n := self.number()) and (s := self.number_suffix(),):
            n = Codon.unwrap(n)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_number(
                n.string,
                s.string if s else "",
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        n = None
        s = None
        return None

    def capture_pattern(self) -> Optional[ast.Expr]:
        # capture_pattern: pattern_capture_target
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if target := self.pattern_capture_target():
            target = Codon.unwrap(target)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IdExpr(
                value=target,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        target = None
        return None

    def pattern_capture_target(self) -> Optional[str]:
        # pattern_capture_target: !"_" NAME !('.' | '(' | '=')
        mark = self._mark()
        if (
            (self.negative_lookahead(self.expect_literal, "_"))
            and (name := self.name())
            and (
                self.negative_lookahead(
                    self._tmp_90,
                )
            )
        ):
            name = Codon.unwrap(name)
            return name.string
        self._reset(mark)
        name = None
        return None

    def wildcard_pattern(self) -> Optional[ast.Expr]:
        # wildcard_pattern: "_"
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if self.expect_literal("_"):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IdExpr(
                value="_",
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        return None

    def value_pattern(self) -> Optional[ast.Expr]:
        # value_pattern: attr !('.' | '(' | '=')
        mark = self._mark()
        if (attr := self.attr()) and (
            self.negative_lookahead(
                self._tmp_91,
            )
        ):
            attr = Codon.unwrap(attr)
            return attr
        self._reset(mark)
        attr = None
        return None

    @memoize_left_rec
    def attr(self) -> Optional[ast.DotExpr]:
        # attr: name_or_attr '.' NAME
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (value := self.name_or_attr()) and (self.expect_literal(".")) and (a := self.name()):
            value = Codon.unwrap(value)
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.DotExpr(
                expr=value,
                member=a.string,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        value = None
        a = None
        return None

    @logger
    def name_or_attr(self) -> Optional[ast.Expr]:
        # name_or_attr: attr | NAME
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if attr := self.attr():
            attr = Codon.unwrap(attr)
            return attr
        self._reset(mark)
        attr = None
        if name := self.name():
            name = Codon.unwrap(name)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IdExpr(
                value=name.string,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        name = None
        return None

    def group_pattern(self) -> Optional[ast.Expr]:
        # group_pattern: '(' pattern ')'
        mark = self._mark()
        if (
            (self.expect_literal("("))
            and (pattern := self.pattern())
            and (self.expect_literal(")"))
        ):
            pattern = Codon.unwrap(pattern)
            return pattern
        self._reset(mark)
        pattern = None
        return None

    def sequence_pattern(self) -> Optional[ast.Expr]:
        # sequence_pattern: '[' maybe_sequence_pattern? ']' | '(' open_sequence_pattern? ')'
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.expect_literal("["))
            and (patterns := self.maybe_sequence_pattern(),)
            and (self.expect_literal("]"))
        ):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.ListExpr(
                items=patterns or [],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        patterns = None
        if (
            (self.expect_literal("("))
            and (patterns := self.open_sequence_pattern(),)
            and (self.expect_literal(")"))
        ):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.TupleExpr(
                items=patterns or [],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        patterns = None
        return None

    def open_sequence_pattern(self) -> Optional[List[ast.Expr]]:
        # open_sequence_pattern: maybe_star_pattern ',' maybe_sequence_pattern?
        mark = self._mark()
        if (
            (pattern := self.maybe_star_pattern())
            and (self.expect_literal(","))
            and (patterns := self.maybe_sequence_pattern(),)
        ):
            pattern = Codon.unwrap(pattern)
            return [pattern] + (patterns or [])
        self._reset(mark)
        pattern = None
        patterns = None
        return None

    def maybe_sequence_pattern(self) -> Optional[List[ast.Expr]]:
        # maybe_sequence_pattern: ','.maybe_star_pattern+ ','?
        mark = self._mark()
        if (patterns := self._gather_92()) and (self.expect_literal(","),):
            patterns = Codon.unwrap(patterns)
            return patterns
        self._reset(mark)
        patterns = None
        return None

    def maybe_star_pattern(self) -> Optional[ast.Expr]:
        # maybe_star_pattern: star_pattern | pattern
        mark = self._mark()
        if star_pattern := self.star_pattern():
            star_pattern = Codon.unwrap(star_pattern)
            return star_pattern
        self._reset(mark)
        star_pattern = None
        if pattern := self.pattern():
            pattern = Codon.unwrap(pattern)
            return pattern
        self._reset(mark)
        pattern = None
        return None

    @memoize
    def star_pattern(self) -> Optional[ast.StarExpr]:
        # star_pattern: '*' pattern_capture_target | '*' wildcard_pattern
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (self.expect_literal("*")) and (target := self.pattern_capture_target()):
            target = Codon.unwrap(target)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.StarExpr(
                expr=ast.IdExpr(
                    value=target,
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                ),
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        target = None
        if (self.expect_literal("*")) and (self.wildcard_pattern()):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.StarExpr(
                expr=ast.IdExpr(
                    value="_",
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                ),
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        return None

    def mapping_pattern(self) -> Optional[ast.DictExpr]:
        # mapping_pattern: '{' '}' | '{' double_star_pattern ','? '}' | '{' items_pattern ',' double_star_pattern ','? '}' | '{' items_pattern ','? '}'
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (self.expect_literal("{")) and (self.expect_literal("}")):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.dict_expr(
                [],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        if (
            (self.expect_literal("{"))
            and (rest := self.double_star_pattern())
            and (self.expect_literal(","),)
            and (self.expect_literal("}"))
        ):
            rest = Codon.unwrap(rest)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.dict_expr(
                [
                    (
                        None,
                        ast.IdExpr(
                            value=rest,
                            lineno=start_lineno,
                            col_offset=start_col_offset,
                            end_lineno=end_lineno,
                            end_col_offset=end_col_offset,
                        ),
                    )
                ],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        rest = None
        if (
            (self.expect_literal("{"))
            and (items := self.items_pattern())
            and (self.expect_literal(","))
            and (rest := self.double_star_pattern())
            and (self.expect_literal(","),)
            and (self.expect_literal("}"))
        ):
            items = Codon.unwrap(items)
            rest = Codon.unwrap(rest)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.dict_expr(
                items
                + [
                    (
                        None,
                        ast.IdExpr(
                            value=rest,
                            lineno=start_lineno,
                            col_offset=start_col_offset,
                            end_lineno=end_lineno,
                            end_col_offset=end_col_offset,
                        ),
                    )
                ],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        items = None
        rest = None
        if (
            (self.expect_literal("{"))
            and (items := self.items_pattern())
            and (self.expect_literal(","),)
            and (self.expect_literal("}"))
        ):
            items = Codon.unwrap(items)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.dict_expr(
                items,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        items = None
        return None

    def items_pattern(self) -> Optional[List[Tuple[ast.Expr, ast.Expr]]]:
        # items_pattern: ','.key_value_pattern+
        mark = self._mark()
        if _gather_94 := self._gather_94():
            _gather_94 = Codon.unwrap(_gather_94)
            return _gather_94
        self._reset(mark)
        _gather_94 = None
        return None

    def key_value_pattern(self) -> Optional[Tuple[ast.Expr, ast.Expr]]:
        # key_value_pattern: (literal_expr | attr) ':' pattern
        mark = self._mark()
        if (key := self._tmp_96()) and (self.expect_literal(":")) and (pattern := self.pattern()):
            key = Codon.unwrap(key)
            pattern = Codon.unwrap(pattern)
            return (key, pattern)
        self._reset(mark)
        key = None
        pattern = None
        return None

    def double_star_pattern(self) -> Optional[str]:
        # double_star_pattern: '**' pattern_capture_target
        mark = self._mark()
        if (self.expect_literal("**")) and (target := self.pattern_capture_target()):
            target = Codon.unwrap(target)
            return target
        self._reset(mark)
        target = None
        return None

    def class_pattern(self) -> Optional[ast.CallExpr]:
        # class_pattern: name_or_attr '(' ')' | name_or_attr '(' positional_patterns ','? ')' | name_or_attr '(' keyword_patterns ','? ')' | name_or_attr '(' positional_patterns ',' keyword_patterns ','? ')' | invalid_class_pattern
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (cls := self.name_or_attr())
            and (self.expect_literal("("))
            and (self.expect_literal(")"))
        ):
            cls = Codon.unwrap(cls)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.call(
                cls,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        cls = None
        if (
            (cls := self.name_or_attr())
            and (self.expect_literal("("))
            and (patterns := self.positional_patterns())
            and (self.expect_literal(","),)
            and (self.expect_literal(")"))
        ):
            cls = Codon.unwrap(cls)
            patterns = Codon.unwrap(patterns)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.call(
                cls,
                patterns,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        cls = None
        patterns = None
        if (
            (cls := self.name_or_attr())
            and (self.expect_literal("("))
            and (keywords := self.keyword_patterns())
            and (self.expect_literal(","),)
            and (self.expect_literal(")"))
        ):
            cls = Codon.unwrap(cls)
            keywords = Codon.unwrap(keywords)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.call(
                cls,
                [],
                [
                    self.call_arg(
                        k,
                        p,
                        lineno=start_lineno,
                        col_offset=start_col_offset,
                        end_lineno=end_lineno,
                        end_col_offset=end_col_offset,
                    )
                    for k, p in keywords
                ],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        cls = None
        keywords = None
        if (
            (cls := self.name_or_attr())
            and (self.expect_literal("("))
            and (patterns := self.positional_patterns())
            and (self.expect_literal(","))
            and (keywords := self.keyword_patterns())
            and (self.expect_literal(","),)
            and (self.expect_literal(")"))
        ):
            cls = Codon.unwrap(cls)
            patterns = Codon.unwrap(patterns)
            keywords = Codon.unwrap(keywords)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.call(
                cls,
                patterns,
                [
                    self.call_arg(
                        k,
                        p,
                        lineno=start_lineno,
                        col_offset=start_col_offset,
                        end_lineno=end_lineno,
                        end_col_offset=end_col_offset,
                    )
                    for k, p in keywords
                ],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        cls = None
        patterns = None
        keywords = None
        if self.call_invalid_rules and (self.invalid_class_pattern()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def positional_patterns(self) -> Optional[List[ast.Expr]]:
        # positional_patterns: ','.pattern+
        mark = self._mark()
        if args := self._gather_97():
            args = Codon.unwrap(args)
            return args
        self._reset(mark)
        args = None
        return None

    def keyword_patterns(self) -> Optional[List[Tuple[str, ast.Expr]]]:
        # keyword_patterns: ','.keyword_pattern+
        mark = self._mark()
        if _gather_99 := self._gather_99():
            _gather_99 = Codon.unwrap(_gather_99)
            return _gather_99
        self._reset(mark)
        _gather_99 = None
        return None

    def keyword_pattern(self) -> Optional[Tuple[str, ast.Expr]]:
        # keyword_pattern: NAME '=' pattern
        mark = self._mark()
        if (arg := self.name()) and (self.expect_literal("=")) and (value := self.pattern()):
            arg = Codon.unwrap(arg)
            value = Codon.unwrap(value)
            return (arg.string, value)
        self._reset(mark)
        arg = None
        value = None
        return None

    def custom_stmt(self) -> Optional[ast.CustomStmt]:
        # custom_stmt: !"_" SOFT_KEYWORD expression ':' block | !"_" SOFT_KEYWORD ':' block
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.negative_lookahead(self.expect_literal, "_"))
            and (a := self.soft_keyword())
            and (e := self.expression())
            and (self.expect_literal(":"))
            and (b := self.block())
        ):
            a = Codon.unwrap(a)
            e = Codon.unwrap(e)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.CustomStmt(
                keyword=a.string,
                expr=e,
                suite=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        e = None
        b = None
        if (
            (self.negative_lookahead(self.expect_literal, "_"))
            and (a := self.soft_keyword())
            and (self.expect_literal(":"))
            and (b := self.block())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.CustomStmt(
                keyword=a.string,
                suite=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        return None

    def type_params(self) -> Optional[List[ast.Param]]:
        # type_params: '[' type_param_seq ']'
        mark = self._mark()
        if (
            (self.expect_literal("["))
            and (t := self.type_param_seq())
            and (self.expect_literal("]"))
        ):
            t = Codon.unwrap(t)
            return t
        self._reset(mark)
        t = None
        return None

    def type_param_seq(self) -> Optional[List[ast.Param]]:
        # type_param_seq: ','.type_param+ ','?
        mark = self._mark()
        if (a := self._gather_101()) and (self.expect_literal(","),):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    @memoize
    def type_param(self) -> Optional[ast.Param]:
        # type_param: NAME type_param_bound?
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.name()) and (b := self.type_param_bound(),):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.Param(
                name=a.string,
                type=b
                or ast.IdExpr(
                    value="type",
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                ),
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        return None

    def type_param_bound(self) -> Optional[ast.Expr]:
        # type_param_bound: ":" expression
        mark = self._mark()
        if (self.expect_literal(":")) and (e := self.expression()):
            e = Codon.unwrap(e)
            return e
        self._reset(mark)
        e = None
        return None

    def expressions(self) -> Optional[ast.Expr]:
        # expressions: expression ((',' expression))+ ','? | expression ',' | expression
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.expression()) and (b := self._loop1_103()) and (self.expect_literal(","),):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.TupleExpr(
                items=[a] + b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (a := self.expression()) and (self.expect_literal(",")):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.TupleExpr(
                items=[a],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if expression := self.expression():
            expression = Codon.unwrap(expression)
            return expression
        self._reset(mark)
        expression = None
        return None

    @memoize
    def expression(self) -> Optional[ast.Expr]:
        # expression: invalid_expression | invalid_legacy_expression | disjunction 'if' disjunction 'else' expression | pipe | lambdef
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if self.call_invalid_rules and (self.invalid_expression()):
            return None  # pragma: no cover
        self._reset(mark)
        if self.call_invalid_rules and (self.invalid_legacy_expression()):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (a := self.disjunction())
            and (self.expect_literal("if"))
            and (b := self.disjunction())
            and (self.expect_literal("else"))
            and (c := self.expression())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            c = Codon.unwrap(c)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IfExpr(
                cond=b,
                ifexpr=a,
                elsexpr=c,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        if pipe := self.pipe():
            pipe = Codon.unwrap(pipe)
            return pipe
        self._reset(mark)
        pipe = None
        if lambdef := self.lambdef():
            lambdef = Codon.unwrap(lambdef)
            return lambdef
        self._reset(mark)
        lambdef = None
        return None

    def yield_expr(self) -> Optional[ast.Expr]:
        # yield_expr: '(' 'yield' ')'
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.expect_literal("("))
            and (self.expect_literal("yield"))
            and (self.expect_literal(")"))
        ):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.YieldExpr(
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        return None

    def star_expressions(self) -> Optional[ast.Expr]:
        # star_expressions: star_expression ((',' star_expression))+ ','? | star_expression ',' | star_expression
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (a := self.star_expression())
            and (b := self._loop1_104())
            and (self.expect_literal(","),)
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.TupleExpr(
                items=[a] + b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (a := self.star_expression()) and (self.expect_literal(",")):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.TupleExpr(
                items=[a],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if star_expression := self.star_expression():
            star_expression = Codon.unwrap(star_expression)
            return star_expression
        self._reset(mark)
        star_expression = None
        return None

    @memoize
    def star_expression(self) -> Optional[ast.Expr]:
        # star_expression: '*' bitwise_or | expression
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (self.expect_literal("*")) and (a := self.bitwise_or()):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.StarExpr(
                expr=a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if expression := self.expression():
            expression = Codon.unwrap(expression)
            return expression
        self._reset(mark)
        expression = None
        return None

    def star_named_expressions(self) -> Optional[List[ast.Expr]]:
        # star_named_expressions: ','.star_named_expression+ ','?
        mark = self._mark()
        if (a := self._gather_105()) and (self.expect_literal(","),):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def star_named_expression(self) -> Optional[ast.Expr]:
        # star_named_expression: '*' bitwise_or | named_expression
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (self.expect_literal("*")) and (a := self.bitwise_or()):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.StarExpr(
                expr=a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if named_expression := self.named_expression():
            named_expression = Codon.unwrap(named_expression)
            return named_expression
        self._reset(mark)
        named_expression = None
        return None

    def assignment_expression(self) -> Optional[ast.Expr]:
        # assignment_expression: NAME ':=' ~ expression
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        cut = False
        if (
            (a := self.name())
            and (self.expect_literal(":="))
            and (cut := True)
            and (b := self.expression())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.AssignExpr(
                var=ast.IdExpr(
                    value=a.string,
                    lineno=a.start[0],
                    col_offset=a.start[1],
                    end_lineno=a.end[0],
                    end_col_offset=a.end[1],
                ),
                expr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        if cut:
            return None
        a = None
        b = None
        return None

    def named_expression(self) -> Optional[ast.Expr]:
        # named_expression: assignment_expression | invalid_named_expression | expression !':='
        mark = self._mark()
        if assignment_expression := self.assignment_expression():
            assignment_expression = Codon.unwrap(assignment_expression)
            return assignment_expression
        self._reset(mark)
        assignment_expression = None
        if self.call_invalid_rules and (self.invalid_named_expression()):
            return None  # pragma: no cover
        self._reset(mark)
        if (a := self.expression()) and (self.negative_lookahead(self.expect_literal, ":=")):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    @memoize
    def pipe(self) -> Optional[ast.Expr]:
        # pipe: disjunction ((pipe_operator disjunction))+ | disjunction
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.disjunction()) and (b := self._loop1_107()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_pipe(
                a,
                b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if disjunction := self.disjunction():
            disjunction = Codon.unwrap(disjunction)
            return disjunction
        self._reset(mark)
        disjunction = None
        return None

    def pipe_operator(self) -> Optional[str]:
        # pipe_operator: '||>' | '|>'
        mark = self._mark()
        if self.expect_literal("||>"):
            return "||>"
        self._reset(mark)
        if self.expect_literal("|>"):
            return "|>"
        self._reset(mark)
        return None

    @memoize
    def disjunction(self) -> Optional[ast.Expr]:
        # disjunction: conjunction (('or' conjunction))+ | conjunction
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.conjunction()) and (b := self._loop1_108()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.binary_chain(
                a,
                [("||", value) for value in b],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if conjunction := self.conjunction():
            conjunction = Codon.unwrap(conjunction)
            return conjunction
        self._reset(mark)
        conjunction = None
        return None

    @memoize
    def conjunction(self) -> Optional[ast.Expr]:
        # conjunction: inversion (('and' inversion))+ | inversion
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.inversion()) and (b := self._loop1_109()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.binary_chain(
                a,
                [("&&", value) for value in b],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if inversion := self.inversion():
            inversion = Codon.unwrap(inversion)
            return inversion
        self._reset(mark)
        inversion = None
        return None

    @memoize
    def inversion(self) -> Optional[ast.Expr]:
        # inversion: 'not' inversion | comparison
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (self.expect_literal("not")) and (a := self.inversion()):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.UnaryExpr(
                op="!",
                expr=a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if comparison := self.comparison():
            comparison = Codon.unwrap(comparison)
            return comparison
        self._reset(mark)
        comparison = None
        return None

    def comparison(self) -> Optional[ast.Expr]:
        # comparison: bitwise_or compare_op_bitwise_or_pair+ | bitwise_or
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.bitwise_or()) and (b := self._loop1_110()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_comparison(
                a,
                b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if bitwise_or := self.bitwise_or():
            bitwise_or = Codon.unwrap(bitwise_or)
            return bitwise_or
        self._reset(mark)
        bitwise_or = None
        return None

    def compare_op_bitwise_or_pair(self) -> Optional[Tuple[str, ast.Expr]]:
        # compare_op_bitwise_or_pair: eq_bitwise_or | noteq_bitwise_or | lte_bitwise_or | lt_bitwise_or | gte_bitwise_or | gt_bitwise_or | notin_bitwise_or | in_bitwise_or | isnot_bitwise_or | is_bitwise_or
        mark = self._mark()
        if eq_bitwise_or := self.eq_bitwise_or():
            eq_bitwise_or = Codon.unwrap(eq_bitwise_or)
            return eq_bitwise_or
        self._reset(mark)
        eq_bitwise_or = None
        if noteq_bitwise_or := self.noteq_bitwise_or():
            noteq_bitwise_or = Codon.unwrap(noteq_bitwise_or)
            return noteq_bitwise_or
        self._reset(mark)
        noteq_bitwise_or = None
        if lte_bitwise_or := self.lte_bitwise_or():
            lte_bitwise_or = Codon.unwrap(lte_bitwise_or)
            return lte_bitwise_or
        self._reset(mark)
        lte_bitwise_or = None
        if lt_bitwise_or := self.lt_bitwise_or():
            lt_bitwise_or = Codon.unwrap(lt_bitwise_or)
            return lt_bitwise_or
        self._reset(mark)
        lt_bitwise_or = None
        if gte_bitwise_or := self.gte_bitwise_or():
            gte_bitwise_or = Codon.unwrap(gte_bitwise_or)
            return gte_bitwise_or
        self._reset(mark)
        gte_bitwise_or = None
        if gt_bitwise_or := self.gt_bitwise_or():
            gt_bitwise_or = Codon.unwrap(gt_bitwise_or)
            return gt_bitwise_or
        self._reset(mark)
        gt_bitwise_or = None
        if notin_bitwise_or := self.notin_bitwise_or():
            notin_bitwise_or = Codon.unwrap(notin_bitwise_or)
            return notin_bitwise_or
        self._reset(mark)
        notin_bitwise_or = None
        if in_bitwise_or := self.in_bitwise_or():
            in_bitwise_or = Codon.unwrap(in_bitwise_or)
            return in_bitwise_or
        self._reset(mark)
        in_bitwise_or = None
        if isnot_bitwise_or := self.isnot_bitwise_or():
            isnot_bitwise_or = Codon.unwrap(isnot_bitwise_or)
            return isnot_bitwise_or
        self._reset(mark)
        isnot_bitwise_or = None
        if is_bitwise_or := self.is_bitwise_or():
            is_bitwise_or = Codon.unwrap(is_bitwise_or)
            return is_bitwise_or
        self._reset(mark)
        is_bitwise_or = None
        return None

    def eq_bitwise_or(self) -> Optional[Tuple[str, ast.Expr]]:
        # eq_bitwise_or: '==' bitwise_or
        mark = self._mark()
        if (self.expect_literal("==")) and (a := self.bitwise_or()):
            a = Codon.unwrap(a)
            return ("==", a)
        self._reset(mark)
        a = None
        return None

    def noteq_bitwise_or(self) -> Optional[Tuple[str, ast.Expr]]:
        # noteq_bitwise_or: '!=' bitwise_or
        mark = self._mark()
        if (self.expect_literal("!=")) and (a := self.bitwise_or()):
            a = Codon.unwrap(a)
            return ("!=", a)
        self._reset(mark)
        a = None
        return None

    def lte_bitwise_or(self) -> Optional[Tuple[str, ast.Expr]]:
        # lte_bitwise_or: '<=' bitwise_or
        mark = self._mark()
        if (self.expect_literal("<=")) and (a := self.bitwise_or()):
            a = Codon.unwrap(a)
            return ("<=", a)
        self._reset(mark)
        a = None
        return None

    def lt_bitwise_or(self) -> Optional[Tuple[str, ast.Expr]]:
        # lt_bitwise_or: '<' bitwise_or
        mark = self._mark()
        if (self.expect_literal("<")) and (a := self.bitwise_or()):
            a = Codon.unwrap(a)
            return ("<", a)
        self._reset(mark)
        a = None
        return None

    def gte_bitwise_or(self) -> Optional[Tuple[str, ast.Expr]]:
        # gte_bitwise_or: '>=' bitwise_or
        mark = self._mark()
        if (self.expect_literal(">=")) and (a := self.bitwise_or()):
            a = Codon.unwrap(a)
            return (">=", a)
        self._reset(mark)
        a = None
        return None

    def gt_bitwise_or(self) -> Optional[Tuple[str, ast.Expr]]:
        # gt_bitwise_or: '>' bitwise_or
        mark = self._mark()
        if (self.expect_literal(">")) and (a := self.bitwise_or()):
            a = Codon.unwrap(a)
            return (">", a)
        self._reset(mark)
        a = None
        return None

    def notin_bitwise_or(self) -> Optional[Tuple[str, ast.Expr]]:
        # notin_bitwise_or: 'not' 'in' bitwise_or
        mark = self._mark()
        if (
            (self.expect_literal("not"))
            and (self.expect_literal("in"))
            and (a := self.bitwise_or())
        ):
            a = Codon.unwrap(a)
            return ("not in", a)
        self._reset(mark)
        a = None
        return None

    def in_bitwise_or(self) -> Optional[Tuple[str, ast.Expr]]:
        # in_bitwise_or: 'in' bitwise_or
        mark = self._mark()
        if (self.expect_literal("in")) and (a := self.bitwise_or()):
            a = Codon.unwrap(a)
            return ("in", a)
        self._reset(mark)
        a = None
        return None

    def isnot_bitwise_or(self) -> Optional[Tuple[str, ast.Expr]]:
        # isnot_bitwise_or: 'is' 'not' bitwise_or
        mark = self._mark()
        if (
            (self.expect_literal("is"))
            and (self.expect_literal("not"))
            and (a := self.bitwise_or())
        ):
            a = Codon.unwrap(a)
            return ("is not", a)
        self._reset(mark)
        a = None
        return None

    def is_bitwise_or(self) -> Optional[Tuple[str, ast.Expr]]:
        # is_bitwise_or: 'is' bitwise_or
        mark = self._mark()
        if (self.expect_literal("is")) and (a := self.bitwise_or()):
            a = Codon.unwrap(a)
            return ("is", a)
        self._reset(mark)
        a = None
        return None

    @memoize_left_rec
    def bitwise_or(self) -> Optional[ast.Expr]:
        # bitwise_or: bitwise_or '|' bitwise_xor | bitwise_xor
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.bitwise_or()) and (self.expect_literal("|")) and (b := self.bitwise_xor()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BinaryExpr(
                lexpr=a,
                op="|",
                rexpr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if bitwise_xor := self.bitwise_xor():
            bitwise_xor = Codon.unwrap(bitwise_xor)
            return bitwise_xor
        self._reset(mark)
        bitwise_xor = None
        return None

    @memoize_left_rec
    def bitwise_xor(self) -> Optional[ast.Expr]:
        # bitwise_xor: bitwise_xor '^' bitwise_and | bitwise_and
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.bitwise_xor()) and (self.expect_literal("^")) and (b := self.bitwise_and()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BinaryExpr(
                lexpr=a,
                op="^",
                rexpr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if bitwise_and := self.bitwise_and():
            bitwise_and = Codon.unwrap(bitwise_and)
            return bitwise_and
        self._reset(mark)
        bitwise_and = None
        return None

    @memoize_left_rec
    def bitwise_and(self) -> Optional[ast.Expr]:
        # bitwise_and: bitwise_and '&' shift_expr | shift_expr
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.bitwise_and()) and (self.expect_literal("&")) and (b := self.shift_expr()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BinaryExpr(
                lexpr=a,
                op="&",
                rexpr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if shift_expr := self.shift_expr():
            shift_expr = Codon.unwrap(shift_expr)
            return shift_expr
        self._reset(mark)
        shift_expr = None
        return None

    @memoize_left_rec
    def shift_expr(self) -> Optional[ast.Expr]:
        # shift_expr: shift_expr '<<' sum | shift_expr '>>' sum | sum
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.shift_expr()) and (self.expect_literal("<<")) and (b := self.sum()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BinaryExpr(
                lexpr=a,
                op="<<",
                rexpr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (a := self.shift_expr()) and (self.expect_literal(">>")) and (b := self.sum()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BinaryExpr(
                lexpr=a,
                op=">>",
                rexpr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if sum := self.sum():
            sum = Codon.unwrap(sum)
            return sum
        self._reset(mark)
        sum = None
        return None

    @memoize_left_rec
    def sum(self) -> Optional[ast.Expr]:
        # sum: sum '+' term | sum '-' term | term
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.sum()) and (self.expect_literal("+")) and (b := self.term()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BinaryExpr(
                lexpr=a,
                op="+",
                rexpr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (a := self.sum()) and (self.expect_literal("-")) and (b := self.term()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BinaryExpr(
                lexpr=a,
                op="-",
                rexpr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if term := self.term():
            term = Codon.unwrap(term)
            return term
        self._reset(mark)
        term = None
        return None

    @memoize_left_rec
    def term(self) -> Optional[ast.Expr]:
        # term: term '*' factor | term '/' factor | term '//' factor | term '%' factor | term '@' factor | factor
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.term()) and (self.expect_literal("*")) and (b := self.factor()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BinaryExpr(
                lexpr=a,
                op="*",
                rexpr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (a := self.term()) and (self.expect_literal("/")) and (b := self.factor()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BinaryExpr(
                lexpr=a,
                op="/",
                rexpr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (a := self.term()) and (self.expect_literal("//")) and (b := self.factor()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BinaryExpr(
                lexpr=a,
                op="//",
                rexpr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (a := self.term()) and (self.expect_literal("%")) and (b := self.factor()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BinaryExpr(
                lexpr=a,
                op="%",
                rexpr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (a := self.term()) and (self.expect_literal("@")) and (b := self.factor()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BinaryExpr(
                lexpr=a,
                op="@",
                rexpr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if factor := self.factor():
            factor = Codon.unwrap(factor)
            return factor
        self._reset(mark)
        factor = None
        return None

    @memoize
    def factor(self) -> Optional[ast.Expr]:
        # factor: '+' factor | '-' factor | '~' factor | power
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (self.expect_literal("+")) and (a := self.factor()):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.UnaryExpr(
                op="+",
                expr=a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if (self.expect_literal("-")) and (a := self.factor()):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.UnaryExpr(
                op="-",
                expr=a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if (self.expect_literal("~")) and (a := self.factor()):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.UnaryExpr(
                op="~",
                expr=a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if power := self.power():
            power = Codon.unwrap(power)
            return power
        self._reset(mark)
        power = None
        return None

    def power(self) -> Optional[ast.Expr]:
        # power: await_primary '**' factor | await_primary
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.await_primary()) and (self.expect_literal("**")) and (b := self.factor()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BinaryExpr(
                lexpr=a,
                op="**",
                rexpr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if await_primary := self.await_primary():
            await_primary = Codon.unwrap(await_primary)
            return await_primary
        self._reset(mark)
        await_primary = None
        return None

    @memoize
    def await_primary(self) -> Optional[ast.Expr]:
        # await_primary: 'await' primary | primary
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (self.expect_literal("await")) and (a := self.primary()):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.AwaitExpr(
                expr=a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if primary := self.primary():
            primary = Codon.unwrap(primary)
            return primary
        self._reset(mark)
        primary = None
        return None

    @memoize_left_rec
    def primary(self) -> Optional[ast.Expr]:
        # primary: primary '.' NAME | primary genexp | primary '(' arguments_with_partial? '...' ')' | primary '(' arguments? ')' | primary '[' slices ']' | atom
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.primary()) and (self.expect_literal(".")) and (b := self.name()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.DotExpr(
                expr=a,
                member=b.string,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (a := self.primary()) and (b := self.genexp()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.call(
                a,
                [b],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (a := self.primary())
            and (self.expect_literal("("))
            and (b := self.arguments_with_partial(),)
            and (self.expect_literal("..."))
            and (self.expect_literal(")"))
        ):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.call(
                a,
                b[0] if b else [],
                b[1] if b else [],
                partial=True,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (a := self.primary())
            and (self.expect_literal("("))
            and (b := self.arguments(),)
            and (self.expect_literal(")"))
        ):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.call(
                a,
                b[0] if b else [],
                b[1] if b else [],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (a := self.primary())
            and (self.expect_literal("["))
            and (b := self.slices())
            and (self.expect_literal("]"))
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IndexExpr(
                expr=a,
                index=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if atom := self.atom():
            atom = Codon.unwrap(atom)
            return atom
        self._reset(mark)
        atom = None
        return None

    def slices(self) -> Optional[ast.Expr]:
        # slices: slice !',' | ','.(slice | starred_expression)+ ','?
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.slice()) and (self.negative_lookahead(self.expect_literal, ",")):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if (a := self._gather_111()) and (self.expect_literal(","),):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.TupleExpr(
                items=a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def slice(self) -> Optional[ast.Expr]:
        # slice: expression? ':' expression? [':' expression?] | named_expression
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (a := self.expression(),)
            and (self.expect_literal(":"))
            and (b := self.expression(),)
            and (c := self._tmp_113(),)
        ):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.SliceExpr(
                start=a,
                stop=b,
                step=c,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        if a := self.named_expression():
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def atom(self) -> Optional[ast.Expr]:
        # atom: NAME | 'True' | 'False' | 'None' | &(STRING | FSTRING_START | STRING_PREFIX) strings | NUMBER '...' NUMBER | any_number | &'(' (tuple | group | genexp) | &'[' (list | listcomp) | &'{' (dict | set | dictcomp | setcomp) | '...'
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if a := self.name():
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IdExpr(
                value=a.string,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if self.expect_literal("True"):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BoolExpr(
                value=True,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        if self.expect_literal("False"):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.BoolExpr(
                value=False,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        if self.expect_literal("None"):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.NoneExpr(
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        if (
            self.positive_lookahead(
                self._tmp_114,
            )
        ) and (strings := self.strings()):
            strings = Codon.unwrap(strings)
            return strings
        self._reset(mark)
        strings = None
        if (a := self.number()) and (self.expect_literal("...")) and (b := self.number()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.RangeExpr(
                start=self.make_number(
                    a.string,
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                ),
                stop=self.make_number(
                    b.string,
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                ),
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if any_number := self.any_number():
            any_number = Codon.unwrap(any_number)
            return any_number
        self._reset(mark)
        any_number = None
        if (self.positive_lookahead(self.expect_literal, "(")) and (_tmp_115 := self._tmp_115()):
            _tmp_115 = Codon.unwrap(_tmp_115)
            return _tmp_115
        self._reset(mark)
        _tmp_115 = None
        if (self.positive_lookahead(self.expect_literal, "[")) and (_tmp_116 := self._tmp_116()):
            _tmp_116 = Codon.unwrap(_tmp_116)
            return _tmp_116
        self._reset(mark)
        _tmp_116 = None
        if (self.positive_lookahead(self.expect_literal, "{")) and (_tmp_117 := self._tmp_117()):
            _tmp_117 = Codon.unwrap(_tmp_117)
            return _tmp_117
        self._reset(mark)
        _tmp_117 = None
        if self.expect_literal("..."):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.EllipsisExpr(
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        return None

    def group(self) -> Optional[ast.Expr]:
        # group: '(' (yield_expr | named_expression) ')' | invalid_group
        mark = self._mark()
        if (self.expect_literal("(")) and (a := self._tmp_118()) and (self.expect_literal(")")):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if self.call_invalid_rules and (self.invalid_group()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def lambdef(self) -> Optional[ast.LambdaExpr]:
        # lambdef: 'lambda' lambda_params? ':' expression
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.expect_literal("lambda"))
            and (a := self.lambda_params(),)
            and (self.expect_literal(":"))
            and (b := self.expression())
        ):
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.LambdaExpr(
                items=a
                or self.make_arguments(
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                ),
                expr=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        return None

    def lambda_params(self) -> Optional:
        # lambda_params: invalid_lambda_parameters | lambda_parameters
        mark = self._mark()
        if self.call_invalid_rules and (self.invalid_lambda_parameters()):
            return None  # pragma: no cover
        self._reset(mark)
        if lambda_parameters := self.lambda_parameters():
            lambda_parameters = Codon.unwrap(lambda_parameters)
            return lambda_parameters
        self._reset(mark)
        lambda_parameters = None
        return None

    def lambda_parameters(self) -> Optional[List[ast.Param]]:
        # lambda_parameters: lambda_slash_no_default lambda_param_no_default* lambda_param_with_default* lambda_star_etc? | lambda_slash_with_default lambda_param_with_default* lambda_star_etc? | lambda_param_no_default+ lambda_param_with_default* lambda_star_etc? | lambda_param_with_default+ lambda_star_etc? | lambda_star_etc
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (a := self.lambda_slash_no_default())
            and (b := self._loop0_119(),)
            and (c := self._loop0_120(),)
            and (d := self.lambda_star_etc(),)
        ):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_arguments(
                pos_only=a,
                param_no_default=b,
                param_default=c,
                after_star=d,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        d = None
        if (
            (a := self.lambda_slash_with_default())
            and (b := self._loop0_121(),)
            and (c := self.lambda_star_etc(),)
        ):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_arguments(
                pos_only_with_default=a,
                param_default=b,
                after_star=c,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        if (
            (a := self._loop1_122())
            and (b := self._loop0_123(),)
            and (c := self.lambda_star_etc(),)
        ):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_arguments(
                param_no_default=a,
                param_default=b,
                after_star=c,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        if (a := self._loop1_124()) and (b := self.lambda_star_etc(),):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_arguments(
                param_default=a,
                after_star=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if a := self.lambda_star_etc():
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.make_arguments(
                after_star=a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def lambda_slash_no_default(self) -> Optional[List[Tuple[ast.Param, ast.Expr | None]]]:
        # lambda_slash_no_default: lambda_param_no_default+ '/' ',' | lambda_param_no_default+ '/' &':'
        mark = self._mark()
        if (a := self._loop1_125()) and (self.expect_literal("/")) and (self.expect_literal(",")):
            a = Codon.unwrap(a)
            return [(p, None) for p in a]
        self._reset(mark)
        a = None
        if (
            (a := self._loop1_126())
            and (self.expect_literal("/"))
            and (self.positive_lookahead(self.expect_literal, ":"))
        ):
            a = Codon.unwrap(a)
            return [(p, None) for p in a]
        self._reset(mark)
        a = None
        return None

    def lambda_slash_with_default(self) -> Optional[List[Tuple[ast.Param, ast.Expr | None]]]:
        # lambda_slash_with_default: lambda_param_no_default* lambda_param_with_default+ '/' ',' | lambda_param_no_default* lambda_param_with_default+ '/' &':'
        mark = self._mark()
        if (
            (a := self._loop0_127(),)
            and (b := self._loop1_128())
            and (self.expect_literal("/"))
            and (self.expect_literal(","))
        ):
            b = Codon.unwrap(b)
            return ([(p, None) for p in a] if a else []) + b
        self._reset(mark)
        a = None
        b = None
        if (
            (a := self._loop0_129(),)
            and (b := self._loop1_130())
            and (self.expect_literal("/"))
            and (self.positive_lookahead(self.expect_literal, ":"))
        ):
            b = Codon.unwrap(b)
            return ([(p, None) for p in a] if a else []) + b
        self._reset(mark)
        a = None
        b = None
        return None

    def lambda_star_etc(
        self,
    ) -> Optional[
        Tuple[
            ast.Param | None,
            List[Tuple[ast.Param, ast.Expr | None]],
            ast.Param | None,
            List[Tuple[ast.Param, ast.Expr | None]],
        ]
    ]:
        # lambda_star_etc: invalid_lambda_star_etc | '*' lambda_param_no_default lambda_param_maybe_default* lambda_kwds? | '*' ',' lambda_param_maybe_default+ lambda_kwds? | lambda_kwds
        mark = self._mark()
        if self.call_invalid_rules and (self.invalid_lambda_star_etc()):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expect_literal("*"))
            and (a := self.lambda_param_no_default())
            and (b := self._loop0_131(),)
            and (c := self.lambda_kwds(),)
        ):
            a = Codon.unwrap(a)
            return (a, b, c, [])
        self._reset(mark)
        a = None
        b = None
        c = None
        if (
            (self.expect_literal("*"))
            and (self.expect_literal(","))
            and (b := self._loop1_132())
            and (c := self.lambda_kwds(),)
        ):
            b = Codon.unwrap(b)
            return (None, b, c, [])
        self._reset(mark)
        b = None
        c = None
        if a := self.lambda_kwds():
            a = Codon.unwrap(a)
            return (None, [], a, [])
        self._reset(mark)
        a = None
        return None

    def lambda_kwds(self) -> Optional[ast.Param]:
        # lambda_kwds: invalid_lambda_kwds | '**' lambda_param_no_default
        mark = self._mark()
        if self.call_invalid_rules and (self.invalid_lambda_kwds()):
            return None  # pragma: no cover
        self._reset(mark)
        if (self.expect_literal("**")) and (a := self.lambda_param_no_default()):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def lambda_param_no_default(self) -> Optional[ast.Param]:
        # lambda_param_no_default: lambda_param ',' | lambda_param &':'
        mark = self._mark()
        if (a := self.lambda_param()) and (self.expect_literal(",")):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if (a := self.lambda_param()) and (self.positive_lookahead(self.expect_literal, ":")):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def lambda_param_with_default(self) -> Optional[Tuple[ast.Param, ast.Expr | None]]:
        # lambda_param_with_default: lambda_param default ',' | lambda_param default &':'
        mark = self._mark()
        if (a := self.lambda_param()) and (c := self.default()) and (self.expect_literal(",")):
            a = Codon.unwrap(a)
            c = Codon.unwrap(c)
            return (a, c)
        self._reset(mark)
        a = None
        c = None
        if (
            (a := self.lambda_param())
            and (c := self.default())
            and (self.positive_lookahead(self.expect_literal, ":"))
        ):
            a = Codon.unwrap(a)
            c = Codon.unwrap(c)
            return (a, c)
        self._reset(mark)
        a = None
        c = None
        return None

    def lambda_param_maybe_default(self) -> Optional[Tuple[ast.Param, ast.Expr | None]]:
        # lambda_param_maybe_default: lambda_param default? ',' | lambda_param default? &':'
        mark = self._mark()
        if (a := self.lambda_param()) and (c := self.default(),) and (self.expect_literal(",")):
            a = Codon.unwrap(a)
            return (a, c)
        self._reset(mark)
        a = None
        c = None
        if (
            (a := self.lambda_param())
            and (c := self.default(),)
            and (self.positive_lookahead(self.expect_literal, ":"))
        ):
            a = Codon.unwrap(a)
            return (a, c)
        self._reset(mark)
        a = None
        c = None
        return None

    def lambda_param(self) -> Optional[ast.Param]:
        # lambda_param: NAME
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if a := self.name():
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.Param(
                name=a.string,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def fstring_mid(self) -> Optional[ast.StringExpr.String]:
        # fstring_mid: fstring_replacement_field | FSTRING_MIDDLE
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if fstring_replacement_field := self.fstring_replacement_field():
            fstring_replacement_field = Codon.unwrap(fstring_replacement_field)
            return fstring_replacement_field
        self._reset(mark)
        fstring_replacement_field = None
        if t := self.fstring_middle():
            t = Codon.unwrap(t)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.StringExpr.String(
                value=t.string,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        t = None
        return None

    def fstring_replacement_field(self) -> Optional[ast.StringExpr.String]:
        # fstring_replacement_field: '{' (yield_expr | star_expressions) "="? fstring_conversion? fstring_full_format_spec? '}' | invalid_replacement_field
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.expect_literal("{"))
            and (a := self._tmp_133())
            and (debug_expr := self.expect_literal("="),)
            and (conversion := self.fstring_conversion(),)
            and (format := self.fstring_full_format_spec(),)
            and (self.expect_literal("}"))
        ):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.StringExpr.String(
                value=debug_expr.string if debug_expr else "",
                expr=a,
                format=ast.StringExpr.FormatSpec(
                    conversion=(conversion or ("r" if debug_expr else "")),
                    spec=format or "",
                ),
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        debug_expr = None
        conversion = None
        format = None
        if self.call_invalid_rules and (self.invalid_replacement_field()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def fstring_conversion(self) -> Optional[str]:
        # fstring_conversion: "!" NAME
        mark = self._mark()
        if (conv_token := self.expect_literal("!")) and (conv := self.name()):
            conv_token = Codon.unwrap(conv_token)
            conv = Codon.unwrap(conv)
            return self.check_fstring_conversion(conv_token, conv)
        self._reset(mark)
        conv_token = None
        conv = None
        return None

    def fstring_full_format_spec(self) -> Optional[str]:
        # fstring_full_format_spec: ':' fstring_format_spec*
        mark = self._mark()
        if (self.expect_literal(":")) and (spec := self._loop0_134(),):
            return "".join(spec)
        self._reset(mark)
        spec = None
        return None

    def fstring_format_spec(self) -> Optional[str]:
        # fstring_format_spec: fstring_options? fstring_width_and_precision? fstring_type?
        # nullable=True
        mark = self._mark()
        if (
            (a := self.fstring_options(),)
            and (b := self.fstring_width_and_precision(),)
            and (c := self.fstring_type(),)
        ):
            return (a or "") + (b or "") + (c.string if c else "")
        self._reset(mark)
        a = None
        b = None
        c = None
        return None

    def fstring_options(self) -> Optional[str]:
        # fstring_options: fstring_options_head? ['+' | '-'] "z"? "#"? "0"?
        # nullable=True
        mark = self._mark()
        if (
            (a := self.fstring_options_head(),)
            and (b := self._tmp_135(),)
            and (c := self.expect_literal("z"),)
            and (d := self.expect_literal("#"),)
            and (e := self.expect_literal("0"),)
        ):
            return (
                (a or "")
                + (b.string if b else "")
                + (c.string if c else "")
                + (d.string if d else "")
                + (e.string if e else "")
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        d = None
        e = None
        return None

    def fstring_options_head(self) -> Optional[str]:
        # fstring_options_head: ANY_BUT_NEWLINE? ('<' | '>' | '=' | '^')
        mark = self._mark()
        if (n := self.any_but_newline(),) and (o := self._tmp_136()):
            o = Codon.unwrap(o)
            return (n.string if n else "") + o.string
        self._reset(mark)
        n = None
        o = None
        return None

    def fstring_width_and_precision(self) -> Optional[str]:
        # fstring_width_and_precision: [NUMBER? fstring_grouping?] fstring_precision_with_grouping?
        # nullable=True
        mark = self._mark()
        if (a := self._tmp_137(),) and (b := self.fstring_precision_with_grouping(),):
            return (a or "") + (b or "")
        self._reset(mark)
        a = None
        b = None
        return None

    def fstring_precision_with_grouping(self) -> Optional[str]:
        # fstring_precision_with_grouping: '.' NUMBER? fstring_grouping? | '.' fstring_grouping
        mark = self._mark()
        if (
            (a := self.expect_literal("."))
            and (b := self.number(),)
            and (c := self.fstring_grouping(),)
        ):
            a = Codon.unwrap(a)
            return a.string + (b.string if b else "") + (c or "")
        self._reset(mark)
        a = None
        b = None
        c = None
        if (a := self.expect_literal(".")) and (b := self.fstring_grouping()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return a.string + b
        self._reset(mark)
        a = None
        b = None
        return None

    def fstring_grouping(self) -> Optional[str]:
        # fstring_grouping: ',' | "_"
        mark = self._mark()
        if a := self.expect_literal(","):
            a = Codon.unwrap(a)
            return a.string
        self._reset(mark)
        a = None
        if a := self.expect_literal("_"):
            a = Codon.unwrap(a)
            return a.string
        self._reset(mark)
        a = None
        return None

    def fstring_type(self) -> Optional:
        # fstring_type: "b" | "c" | "d" | "e" | "E" | "f" | "F" | "g" | "G" | "n" | "o" | "s" | "x" | "X" | '%'
        mark = self._mark()
        if literal := self.expect_literal("b"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("c"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("d"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("e"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("E"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("f"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("F"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("g"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("G"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("n"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("o"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("s"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("x"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("X"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("%"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    @memoize
    def strings(self) -> Optional[ast.Expr]:
        # strings: (any_string)+
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if a := self._loop1_138():
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.generate_ast_for_string(
                a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def any_string(self) -> Optional[ast.StringExpr]:
        # any_string: fstring | STRING_PREFIX? STRING
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if f := self.fstring():
            f = Codon.unwrap(f)
            return f
        self._reset(mark)
        f = None
        if (p := self.string_prefix(),) and (s := self.string()):
            s = Codon.unwrap(s)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.fix_string(
                s,
                p.string if p else "",
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        p = None
        s = None
        return None

    def list(self) -> Optional[ast.Expr]:
        # list: '[' star_named_expressions? ']'
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.expect_literal("["))
            and (a := self.star_named_expressions(),)
            and (self.expect_literal("]"))
        ):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.ListExpr(
                items=a or [],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def tuple(self) -> Optional[ast.Expr]:
        # tuple: '(' [star_named_expression ',' star_named_expressions?] ')'
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (self.expect_literal("(")) and (a := self._tmp_139(),) and (self.expect_literal(")")):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.TupleExpr(
                items=a or [],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def set(self) -> Optional[ast.Expr]:
        # set: '{' star_named_expressions '}'
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.expect_literal("{"))
            and (a := self.star_named_expressions())
            and (self.expect_literal("}"))
        ):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.SetExpr(
                items=a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def dict(self) -> Optional[ast.Expr]:
        # dict: '{' double_starred_kvpairs? '}' | '{' invalid_double_starred_kvpairs '}'
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.expect_literal("{"))
            and (a := self.double_starred_kvpairs(),)
            and (self.expect_literal("}"))
        ):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.dict_expr(
                a or [],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if (
            self.call_invalid_rules
            and (self.expect_literal("{"))
            and (self.invalid_double_starred_kvpairs())
            and (self.expect_literal("}"))
        ):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def double_starred_kvpairs(self) -> Optional[List[Tuple[ast.Expr | None, ast.Expr]]]:
        # double_starred_kvpairs: ','.double_starred_kvpair+ ','?
        mark = self._mark()
        if (a := self._gather_140()) and (self.expect_literal(","),):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def double_starred_kvpair(self) -> Optional[Tuple[ast.Expr | None, ast.Expr]]:
        # double_starred_kvpair: '**' bitwise_or | kvpair
        mark = self._mark()
        if (self.expect_literal("**")) and (a := self.bitwise_or()):
            a = Codon.unwrap(a)
            return (None, a)
        self._reset(mark)
        a = None
        if kvpair := self.kvpair():
            kvpair = Codon.unwrap(kvpair)
            return kvpair
        self._reset(mark)
        kvpair = None
        return None

    def kvpair(self) -> Optional[Tuple[ast.Expr | None, ast.Expr]]:
        # kvpair: expression ':' expression
        mark = self._mark()
        if (a := self.expression()) and (self.expect_literal(":")) and (b := self.expression()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return (a, b)
        self._reset(mark)
        a = None
        b = None
        return None

    def for_if_clauses(self) -> Optional[List[Comprehension]]:
        # for_if_clauses: for_if_clause+
        mark = self._mark()
        if a := self._loop1_142():
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def for_if_clause(self) -> Optional[Comprehension]:
        # for_if_clause: 'async' 'for' star_targets 'in' ~ disjunction (('if' disjunction))* | 'for' star_targets 'in' ~ disjunction (('if' disjunction))* | invalid_for_target
        mark = self._mark()
        cut = False
        if (
            (self.expect_literal("async"))
            and (self.expect_literal("for"))
            and (a := self.star_targets())
            and (self.expect_literal("in"))
            and (cut := True)
            and (b := self.disjunction())
            and (c := self._loop0_143(),)
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return Comprehension(target=a, iter=b, ifs=c, is_async=True)
        self._reset(mark)
        if cut:
            return None
        a = None
        b = None
        c = None
        cut = False
        if (
            (self.expect_literal("for"))
            and (a := self.star_targets())
            and (self.expect_literal("in"))
            and (cut := True)
            and (b := self.disjunction())
            and (c := self._loop0_144(),)
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return Comprehension(target=a, iter=b, ifs=c, is_async=False)
        self._reset(mark)
        if cut:
            return None
        a = None
        b = None
        c = None
        if self.call_invalid_rules and (self.invalid_for_target()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def listcomp(self) -> Optional[ast.GeneratorExpr]:
        # listcomp: '[' named_expression for_if_clauses ']' | invalid_comprehension
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.expect_literal("["))
            and (a := self.named_expression())
            and (b := self.for_if_clauses())
            and (self.expect_literal("]"))
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.generator(
                ast.GeneratorExpr.Kind.ListGenerator,
                a,
                b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if self.call_invalid_rules and (self.invalid_comprehension()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def setcomp(self) -> Optional[ast.GeneratorExpr]:
        # setcomp: '{' named_expression for_if_clauses '}' | invalid_comprehension
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.expect_literal("{"))
            and (a := self.named_expression())
            and (b := self.for_if_clauses())
            and (self.expect_literal("}"))
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.generator(
                ast.GeneratorExpr.Kind.SetGenerator,
                a,
                b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if self.call_invalid_rules and (self.invalid_comprehension()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def genexp(self) -> Optional[ast.GeneratorExpr]:
        # genexp: '(' (assignment_expression | expression !':=') for_if_clauses ')' | invalid_comprehension
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.expect_literal("("))
            and (a := self._tmp_145())
            and (b := self.for_if_clauses())
            and (self.expect_literal(")"))
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.generator(
                ast.GeneratorExpr.Kind.Generator,
                a,
                b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if self.call_invalid_rules and (self.invalid_comprehension()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def dictcomp(self) -> Optional[ast.GeneratorExpr]:
        # dictcomp: '{' kvpair for_if_clauses '}' | invalid_dict_comprehension
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (self.expect_literal("{"))
            and (a := self.kvpair())
            and (b := self.for_if_clauses())
            and (self.expect_literal("}"))
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.generator(
                ast.GeneratorExpr.Kind.DictGenerator,
                a[1],
                b,
                key=a[0],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if self.call_invalid_rules and (self.invalid_dict_comprehension()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    @memoize
    def arguments_with_partial(self) -> Optional[Tuple[List[ast.Expr], List[ast.CallExpr.Arg]]]:
        # arguments_with_partial: args ',' &'...' | invalid_arguments
        mark = self._mark()
        if (
            (a := self.args())
            and (self.expect_literal(","))
            and (self.positive_lookahead(self.expect_literal, "..."))
        ):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if self.call_invalid_rules and (self.invalid_arguments()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    @memoize
    def arguments(self) -> Optional[Tuple[List[ast.Expr], List[ast.CallExpr.Arg]]]:
        # arguments: args ','? &')' | invalid_arguments
        mark = self._mark()
        if (
            (a := self.args())
            and (self.expect_literal(","),)
            and (self.positive_lookahead(self.expect_literal, ")"))
        ):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if self.call_invalid_rules and (self.invalid_arguments()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def args(self) -> Optional[Tuple[List[ast.Expr], List[ast.CallExpr.Arg]]]:
        # args: ','.(starred_expression | (assignment_expression | expression !':=') !'=')+ [',' kwargs] | kwargs
        mark = self._mark()
        if (a := self._gather_146()) and (b := self._tmp_148(),):
            a = Codon.unwrap(a)
            return (
                a + ([e for e in b if isinstance(e, ast.StarExpr)] if b else []),
                ([e for e in b if isinstance(e, ast.CallExpr.Arg)] if b else []),
            )
        self._reset(mark)
        a = None
        b = None
        if a := self.kwargs():
            a = Codon.unwrap(a)
            return (
                [e for e in a if isinstance(e, ast.StarExpr)],
                [e for e in a if isinstance(e, ast.CallExpr.Arg)],
            )
        self._reset(mark)
        a = None
        return None

    def kwargs(self) -> Optional[List[object]]:
        # kwargs: ','.kwarg_or_starred+ ',' ','.kwarg_or_double_starred+ | ','.kwarg_or_starred+ | ','.kwarg_or_double_starred+
        mark = self._mark()
        if (a := self._gather_149()) and (self.expect_literal(",")) and (b := self._gather_151()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return a + b
        self._reset(mark)
        a = None
        b = None
        if _gather_153 := self._gather_153():
            _gather_153 = Codon.unwrap(_gather_153)
            return _gather_153
        self._reset(mark)
        _gather_153 = None
        if _gather_155 := self._gather_155():
            _gather_155 = Codon.unwrap(_gather_155)
            return _gather_155
        self._reset(mark)
        _gather_155 = None
        return None

    def starred_expression(self) -> Optional[ast.Expr]:
        # starred_expression: invalid_starred_expression | '*' expression | invalid_star
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if self.call_invalid_rules and (self.invalid_starred_expression()):
            return None  # pragma: no cover
        self._reset(mark)
        if (self.expect_literal("*")) and (a := self.expression()):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.StarExpr(
                expr=a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if self.call_invalid_rules and (self.invalid_star()):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def kwarg_or_starred(self) -> Optional[object]:
        # kwarg_or_starred: invalid_kwarg | NAME '=' expression | starred_expression
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if self.call_invalid_rules and (self.invalid_kwarg()):
            return None  # pragma: no cover
        self._reset(mark)
        if (a := self.name()) and (self.expect_literal("=")) and (b := self.expression()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.call_arg(
                a.string,
                b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if a := self.starred_expression():
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def kwarg_or_double_starred(self) -> Optional[object]:
        # kwarg_or_double_starred: invalid_kwarg | NAME '=' expression | '**' expression
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if self.call_invalid_rules and (self.invalid_kwarg()):
            return None  # pragma: no cover
        self._reset(mark)
        if (a := self.name()) and (self.expect_literal("=")) and (b := self.expression()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.call_arg(
                a.string,
                b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (self.expect_literal("**")) and (a := self.expression()):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.call_arg(
                "",
                ast.KeywordStarExpr(
                    expr=a,
                    lineno=start_lineno,
                    col_offset=start_col_offset,
                    end_lineno=end_lineno,
                    end_col_offset=end_col_offset,
                ),
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def star_targets(self) -> Optional[ast.Expr]:
        # star_targets: star_target !',' | star_target ((',' star_target))* ','?
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (a := self.star_target()) and (self.negative_lookahead(self.expect_literal, ",")):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if (a := self.star_target()) and (b := self._loop0_157(),) and (self.expect_literal(","),):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.TupleExpr(
                items=[a] + b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        return None

    def star_targets_list_seq(self) -> Optional[List[ast.Expr]]:
        # star_targets_list_seq: ','.star_target+ ','?
        mark = self._mark()
        if (a := self._gather_158()) and (self.expect_literal(","),):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def star_targets_tuple_seq(self) -> Optional[List[ast.Expr]]:
        # star_targets_tuple_seq: star_target ((',' star_target))+ ','? | star_target ','
        mark = self._mark()
        if (a := self.star_target()) and (b := self._loop1_160()) and (self.expect_literal(","),):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return [a] + b
        self._reset(mark)
        a = None
        b = None
        if (a := self.star_target()) and (self.expect_literal(",")):
            a = Codon.unwrap(a)
            return [a]
        self._reset(mark)
        a = None
        return None

    @memoize
    def star_target(self) -> Optional[ast.Expr]:
        # star_target: '*' (!'*' star_target) | target_with_star_atom
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (self.expect_literal("*")) and (a := self._tmp_161()):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.StarExpr(
                expr=a,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if target_with_star_atom := self.target_with_star_atom():
            target_with_star_atom = Codon.unwrap(target_with_star_atom)
            return target_with_star_atom
        self._reset(mark)
        target_with_star_atom = None
        return None

    @memoize
    def target_with_star_atom(self) -> Optional[ast.Expr]:
        # target_with_star_atom: t_primary '.' NAME !t_lookahead | t_primary '[' slices ']' !t_lookahead | star_atom
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (a := self.t_primary())
            and (self.expect_literal("."))
            and (b := self.name())
            and (
                self.negative_lookahead(
                    self.t_lookahead,
                )
            )
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.DotExpr(
                expr=a,
                member=b.string,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (a := self.t_primary())
            and (self.expect_literal("["))
            and (b := self.slices())
            and (self.expect_literal("]"))
            and (
                self.negative_lookahead(
                    self.t_lookahead,
                )
            )
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IndexExpr(
                expr=a,
                index=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if star_atom := self.star_atom():
            star_atom = Codon.unwrap(star_atom)
            return star_atom
        self._reset(mark)
        star_atom = None
        return None

    def star_atom(self) -> Optional[ast.Expr]:
        # star_atom: NAME | '(' target_with_star_atom ')' | '(' star_targets_tuple_seq? ')' | '[' star_targets_list_seq? ']'
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if a := self.name():
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IdExpr(
                value=a.string,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if (
            (self.expect_literal("("))
            and (a := self.target_with_star_atom())
            and (self.expect_literal(")"))
        ):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if (
            (self.expect_literal("("))
            and (a := self.star_targets_tuple_seq(),)
            and (self.expect_literal(")"))
        ):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.TupleExpr(
                items=a or [],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if (
            (self.expect_literal("["))
            and (a := self.star_targets_list_seq(),)
            and (self.expect_literal("]"))
        ):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.ListExpr(
                items=a or [],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def single_target(self) -> Optional[ast.Expr]:
        # single_target: single_subscript_attribute_target | NAME | '(' single_target ')'
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if single_subscript_attribute_target := self.single_subscript_attribute_target():
            single_subscript_attribute_target = Codon.unwrap(single_subscript_attribute_target)
            return single_subscript_attribute_target
        self._reset(mark)
        single_subscript_attribute_target = None
        if a := self.name():
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IdExpr(
                value=a.string,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if (
            (self.expect_literal("("))
            and (a := self.single_target())
            and (self.expect_literal(")"))
        ):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def single_subscript_attribute_target(self) -> Optional[ast.Expr]:
        # single_subscript_attribute_target: t_primary '.' NAME !t_lookahead | t_primary '[' slices ']' !t_lookahead
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (a := self.t_primary())
            and (self.expect_literal("."))
            and (b := self.name())
            and (
                self.negative_lookahead(
                    self.t_lookahead,
                )
            )
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.DotExpr(
                expr=a,
                member=b.string,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (a := self.t_primary())
            and (self.expect_literal("["))
            and (b := self.slices())
            and (self.expect_literal("]"))
            and (
                self.negative_lookahead(
                    self.t_lookahead,
                )
            )
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IndexExpr(
                expr=a,
                index=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        return None

    @memoize_left_rec
    def t_primary(self) -> Optional[ast.Expr]:
        # t_primary: t_primary '.' NAME &t_lookahead | t_primary '[' slices ']' &t_lookahead | t_primary genexp &t_lookahead | t_primary '(' arguments_with_partial? '...' ')' &t_lookahead | t_primary '(' arguments? ')' &t_lookahead | atom &t_lookahead
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (a := self.t_primary())
            and (self.expect_literal("."))
            and (b := self.name())
            and (
                self.positive_lookahead(
                    self.t_lookahead,
                )
            )
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.DotExpr(
                expr=a,
                member=b.string,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (a := self.t_primary())
            and (self.expect_literal("["))
            and (b := self.slices())
            and (self.expect_literal("]"))
            and (
                self.positive_lookahead(
                    self.t_lookahead,
                )
            )
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IndexExpr(
                expr=a,
                index=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (a := self.t_primary())
            and (b := self.genexp())
            and (
                self.positive_lookahead(
                    self.t_lookahead,
                )
            )
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.call(
                a,
                [b],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (a := self.t_primary())
            and (self.expect_literal("("))
            and (b := self.arguments_with_partial(),)
            and (self.expect_literal("..."))
            and (self.expect_literal(")"))
            and (
                self.positive_lookahead(
                    self.t_lookahead,
                )
            )
        ):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.call(
                a,
                b[0] if b else [],
                b[1] if b else [],
                partial=True,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (a := self.t_primary())
            and (self.expect_literal("("))
            and (b := self.arguments(),)
            and (self.expect_literal(")"))
            and (
                self.positive_lookahead(
                    self.t_lookahead,
                )
            )
        ):
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return self.call(
                a,
                b[0] if b else [],
                b[1] if b else [],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (a := self.atom()) and (
            self.positive_lookahead(
                self.t_lookahead,
            )
        ):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def t_lookahead(self) -> Optional:
        # t_lookahead: '(' | '[' | '.'
        mark = self._mark()
        if literal := self.expect_literal("("):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("["):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("."):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def del_targets(self) -> Optional[List[ast.Expr]]:
        # del_targets: ','.del_target+ ','?
        mark = self._mark()
        if (a := self._gather_162()) and (self.expect_literal(","),):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    @memoize
    def del_target(self) -> Optional[ast.Expr]:
        # del_target: t_primary '.' NAME !t_lookahead | t_primary '[' slices ']' !t_lookahead | del_t_atom
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (a := self.t_primary())
            and (self.expect_literal("."))
            and (b := self.name())
            and (
                self.negative_lookahead(
                    self.t_lookahead,
                )
            )
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.DotExpr(
                expr=a,
                member=b.string,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (a := self.t_primary())
            and (self.expect_literal("["))
            and (b := self.slices())
            and (self.expect_literal("]"))
            and (
                self.negative_lookahead(
                    self.t_lookahead,
                )
            )
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IndexExpr(
                expr=a,
                index=b,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        if del_t_atom := self.del_t_atom():
            del_t_atom = Codon.unwrap(del_t_atom)
            return del_t_atom
        self._reset(mark)
        del_t_atom = None
        return None

    def del_t_atom(self) -> Optional[ast.Expr]:
        # del_t_atom: NAME | '(' del_target ')' | '(' del_targets? ')' | '[' del_targets? ']'
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if a := self.name():
            a = Codon.unwrap(a)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.IdExpr(
                value=a.string,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if (self.expect_literal("(")) and (a := self.del_target()) and (self.expect_literal(")")):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if (
            (self.expect_literal("("))
            and (a := self.del_targets(),)
            and (self.expect_literal(")"))
        ):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.TupleExpr(
                items=a or [],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        if (
            (self.expect_literal("["))
            and (a := self.del_targets(),)
            and (self.expect_literal("]"))
        ):
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            return ast.ListExpr(
                items=a or [],
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        return None

    def invalid_arguments(self) -> None:
        # invalid_arguments: ((','.(starred_expression | (assignment_expression | expression !':=') !'=')+ ',' kwargs) | kwargs) ',' ','.(starred_expression !'=')+ | expression for_if_clauses ',' [args | expression for_if_clauses] | NAME '=' expression for_if_clauses | [(args ',')] NAME '=' &(',' | ')') | args for_if_clauses | args ',' expression for_if_clauses | args ',' args
        mark = self._mark()
        if (self._tmp_164()) and (a := self.expect_literal(",")) and (self._gather_165()):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_starting_from(
                "iterable argument unpacking follows keyword argument unpacking",
                a,
            )
        self._reset(mark)
        a = None
        if (
            (a := self.expression())
            and (b := self.for_if_clauses())
            and (self.expect_literal(","))
            and (self._tmp_167(),)
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.raise_syntax_error_known_range(
                "Generator expression must be parenthesized",
                a,
                (b[-1].ifs[-1] if b[-1].ifs else b[-1].iter),
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (a := self.name())
            and (b := self.expect_literal("="))
            and (self.expression())
            and (self.for_if_clauses())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.raise_syntax_error_known_range(
                "invalid syntax. Maybe you meant '==' or ':=' instead of '='?", a, b
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (self._tmp_168(),)
            and (a := self.name())
            and (b := self.expect_literal("="))
            and (
                self.positive_lookahead(
                    self._tmp_169,
                )
            )
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.raise_syntax_error_known_range("expected argument value expression", a, b)
        self._reset(mark)
        a = None
        b = None
        if (a := self.args()) and (b := self.for_if_clauses()):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return (
                self.raise_syntax_error_known_range(
                    "Generator expression must be parenthesized",
                    a[0][-1],
                    (b[-1].ifs[-1] if b[-1].ifs else b[-1].iter),
                )
                if len(a[0]) > 1
                else None
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (self.args())
            and (self.expect_literal(","))
            and (a := self.expression())
            and (b := self.for_if_clauses())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.raise_syntax_error_known_range(
                "Generator expression must be parenthesized",
                a,
                (b[-1].ifs[-1] if b[-1].ifs else b[-1].iter),
            )
        self._reset(mark)
        a = None
        b = None
        if (a := self.args()) and (self.expect_literal(",")) and (self.args()):
            a = Codon.unwrap(a)
            return self.raise_syntax_error(
                (
                    "positional argument follows keyword argument unpacking"
                    if a[1][-1].arg is None
                    else "positional argument follows keyword argument"
                ),
            )
        self._reset(mark)
        a = None
        return None

    def invalid_kwarg(self) -> None:
        # invalid_kwarg: ('True' | 'False' | 'None') '=' | NAME '=' expression for_if_clauses | !(NAME '=') expression '=' | '**' expression '=' expression
        mark = self._mark()
        if (a := self._tmp_170()) and (b := self.expect_literal("=")):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.raise_syntax_error_known_range(f"cannot assign to {a.string}", a, b)
        self._reset(mark)
        a = None
        b = None
        if (
            (a := self.name())
            and (b := self.expect_literal("="))
            and (self.expression())
            and (self.for_if_clauses())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.raise_syntax_error_known_range(
                "invalid syntax. Maybe you meant '==' or ':=' instead of '='?", a, b
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (
                self.negative_lookahead(
                    self._tmp_171,
                )
            )
            and (a := self.expression())
            and (b := self.expect_literal("="))
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.raise_syntax_error_known_range(
                'expression cannot contain assignment, perhaps you meant "=="?',
                a,
                b,
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (a := self.expect_literal("**"))
            and (self.expression())
            and (self.expect_literal("="))
            and (b := self.expression())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.raise_syntax_error_known_range(
                "cannot assign to keyword argument unpacking", a, b
            )
        self._reset(mark)
        a = None
        b = None
        return None

    def expression_without_invalid(self) -> Optional[ast.Expr]:
        # expression_without_invalid: disjunction 'if' disjunction 'else' expression | pipe | lambdef
        _prev_call_invalid = self.call_invalid_rules
        self.call_invalid_rules = False
        mark = self._mark()
        tok = self._tokenizer.peek()
        start_lineno, start_col_offset = tok.start
        if (
            (a := self.disjunction())
            and (self.expect_literal("if"))
            and (b := self.disjunction())
            and (self.expect_literal("else"))
            and (c := self.expression())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            c = Codon.unwrap(c)
            tok = self._tokenizer.get_last_non_whitespace_token()
            end_lineno, end_col_offset = tok.end
            self.call_invalid_rules = _prev_call_invalid
            return ast.IfExpr(
                cond=a,
                ifexpr=b,
                elsexpr=c,
                lineno=start_lineno,
                col_offset=start_col_offset,
                end_lineno=end_lineno,
                end_col_offset=end_col_offset,
            )
        self._reset(mark)
        a = None
        b = None
        c = None
        if pipe := self.pipe():
            pipe = Codon.unwrap(pipe)
            self.call_invalid_rules = _prev_call_invalid
            return pipe
        self._reset(mark)
        pipe = None
        if lambdef := self.lambdef():
            lambdef = Codon.unwrap(lambdef)
            self.call_invalid_rules = _prev_call_invalid
            return lambdef
        self._reset(mark)
        lambdef = None
        self.call_invalid_rules = _prev_call_invalid
        return None

    def invalid_legacy_expression(self) -> None:
        # invalid_legacy_expression: NAME !'(' star_expressions
        mark = self._mark()
        if (
            (a := self.name())
            and (self.negative_lookahead(self.expect_literal, "("))
            and (b := self.star_expressions())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return (
                self.raise_syntax_error_known_range(
                    f"Missing parentheses in call to '{a.string}' . Did you mean {a.string}(...)?",
                    a,
                    b,
                )
                if a.string in ("exec", "print")
                else None
            )
        self._reset(mark)
        a = None
        b = None
        return None

    def invalid_expression(self) -> None:
        # invalid_expression: !(NAME STRING | SOFT_KEYWORD) pipe expression_without_invalid | disjunction 'if' disjunction !('else' | ':') | 'lambda' lambda_params? ':' &(FSTRING_MIDDLE | fstring_replacement_field)
        mark = self._mark()
        if (
            (
                self.negative_lookahead(
                    self._tmp_172,
                )
            )
            and (a := self.pipe())
            and (b := self.expression_without_invalid())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return (
                self.raise_syntax_error_known_range(
                    "invalid syntax. Perhaps you forgot a comma?", a, b
                )
                if not isinstance(a, ast.IdExpr) or a.value not in ("print", "exec")
                else None
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (a := self.disjunction())
            and (self.expect_literal("if"))
            and (b := self.disjunction())
            and (
                self.negative_lookahead(
                    self._tmp_173,
                )
            )
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.raise_syntax_error_known_range(
                "expected 'else' after 'if' expression", a, b
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (a := self.expect_literal("lambda"))
            and (self.lambda_params(),)
            and (b := self.expect_literal(":"))
            and (
                self.positive_lookahead(
                    self._tmp_174,
                )
            )
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.raise_syntax_error_known_range(
                "f-string: lambda expressions are not allowed without parentheses", a, b
            )
        self._reset(mark)
        a = None
        b = None
        return None

    @memoize
    def invalid_named_expression(self) -> None:
        # invalid_named_expression: expression ':=' expression | NAME '=' bitwise_or !('=' | ':=') | !((list | tuple | genexp) | ('True' | 'None' | 'False')) bitwise_or '=' bitwise_or !('=' | ':=')
        mark = self._mark()
        if (a := self.expression()) and (self.expect_literal(":=")) and (self.expression()):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                f"cannot use assignment expressions with {self.get_expr_name(a)}", a
            )
        self._reset(mark)
        a = None
        if (
            (a := self.name())
            and (self.expect_literal("="))
            and (b := self.bitwise_or())
            and (
                self.negative_lookahead(
                    self._tmp_175,
                )
            )
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return (
                None
                if self.in_recursive_rule
                else self.raise_syntax_error_known_range(
                    "invalid syntax. Maybe you meant '==' or ':=' instead of '='?", a, b
                )
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (
                self.negative_lookahead(
                    self._tmp_176,
                )
            )
            and (a := self.bitwise_or())
            and (self.expect_literal("="))
            and (self.bitwise_or())
            and (
                self.negative_lookahead(
                    self._tmp_177,
                )
            )
        ):
            a = Codon.unwrap(a)
            return (
                None
                if self.in_recursive_rule
                else self.raise_syntax_error_known_location(
                    f"cannot assign to {self.get_expr_name(a)} here. Maybe you meant '==' instead of '='?",
                    a,
                )
            )
        self._reset(mark)
        a = None
        return None

    def invalid_assignment(self) -> None:
        # invalid_assignment: invalid_ann_assign_target ':' expression | star_named_expression ',' star_named_expressions* ':' expression | expression ':' expression | ((star_targets '='))* star_expressions '=' | ((star_targets '='))* yield_expr '=' | star_expressions augassign (yield_expr | star_expressions)
        mark = self._mark()
        if (
            self.call_invalid_rules
            and (a := self.invalid_ann_assign_target())
            and (self.expect_literal(":"))
            and (self.expression())
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                f"only single target (not {self.get_expr_name(a)}) can be annotated", a
            )
        self._reset(mark)
        a = None
        if (
            (a := self.star_named_expression())
            and (self.expect_literal(","))
            and (self._loop0_178(),)
            and (self.expect_literal(":"))
            and (self.expression())
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "only single target (not tuple) can be annotated", a
            )
        self._reset(mark)
        a = None
        if (a := self.expression()) and (self.expect_literal(":")) and (self.expression()):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location("illegal target for annotation", a)
        self._reset(mark)
        a = None
        if (self._loop0_179(),) and (a := self.star_expressions()) and (self.expect_literal("=")):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_invalid_target(Target.STAR_TARGETS, a)
        self._reset(mark)
        a = None
        if (self._loop0_180(),) and (a := self.yield_expr()) and (self.expect_literal("=")):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "assignment to yield expression not possible", a
            )
        self._reset(mark)
        a = None
        if (a := self.star_expressions()) and (self.augassign()) and (self._tmp_181()):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                f"'{self.get_expr_name(a)}' is an illegal expression for augmented assignment", a
            )
        self._reset(mark)
        a = None
        return None

    def invalid_ann_assign_target(self) -> Optional[ast.Expr]:
        # invalid_ann_assign_target: list | tuple | '(' invalid_ann_assign_target ')'
        mark = self._mark()
        if a := self.list():
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if a := self.tuple():
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if (
            self.call_invalid_rules
            and (self.expect_literal("("))
            and (a := self.invalid_ann_assign_target())
            and (self.expect_literal(")"))
        ):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def invalid_del_stmt(self) -> None:
        # invalid_del_stmt: 'del' star_expressions
        mark = self._mark()
        if (self.expect_literal("del")) and (a := self.star_expressions()):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_invalid_target(Target.DEL_TARGETS, a)
        self._reset(mark)
        a = None
        return None

    def invalid_block(self) -> None:
        # invalid_block: NEWLINE !INDENT
        mark = self._mark()
        if (self.expect_type(tokenize.Tokens.NEWLINE)) and (
            self.negative_lookahead(self.expect_type, tokenize.Tokens.INDENT)
        ):
            return self.raise_indentation_error("expected an indented block")
        self._reset(mark)
        return None

    def invalid_comprehension(self) -> None:
        # invalid_comprehension: ('[' | '(' | '{') starred_expression for_if_clauses | ('[' | '{') star_named_expression ',' star_named_expressions for_if_clauses | ('[' | '{') star_named_expression ',' for_if_clauses
        mark = self._mark()
        if (self._tmp_182()) and (a := self.starred_expression()) and (self.for_if_clauses()):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "iterable unpacking cannot be used in comprehension", a
            )
        self._reset(mark)
        a = None
        if (
            (self._tmp_183())
            and (a := self.star_named_expression())
            and (self.expect_literal(","))
            and (b := self.star_named_expressions())
            and (self.for_if_clauses())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.raise_syntax_error_known_range(
                "did you forget parentheses around the comprehension target?", a, b[-1]
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (self._tmp_184())
            and (a := self.star_named_expression())
            and (b := self.expect_literal(","))
            and (self.for_if_clauses())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.raise_syntax_error_known_range(
                "did you forget parentheses around the comprehension target?", a, b
            )
        self._reset(mark)
        a = None
        b = None
        return None

    def invalid_dict_comprehension(self) -> None:
        # invalid_dict_comprehension: '{' '**' bitwise_or for_if_clauses '}'
        mark = self._mark()
        if (
            (self.expect_literal("{"))
            and (a := self.expect_literal("**"))
            and (self.bitwise_or())
            and (self.for_if_clauses())
            and (self.expect_literal("}"))
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "dict unpacking cannot be used in dict comprehension", a
            )
        self._reset(mark)
        a = None
        return None

    def invalid_parameters(self) -> None:
        # invalid_parameters: "/" ',' | (slash_no_default | slash_with_default) param_maybe_default* '/' | slash_no_default? param_no_default* invalid_parameters_helper param_no_default | param_no_default* '(' param_no_default+ ','? ')' | [(slash_no_default | slash_with_default)] param_maybe_default* '*' (',' | param_no_default) param_maybe_default* '/' | param_maybe_default+ '/' '*'
        mark = self._mark()
        if (a := self.expect_literal("/")) and (self.expect_literal(",")):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "at least one argument must precede /", a
            )
        self._reset(mark)
        a = None
        if (self._tmp_185()) and (self._loop0_186(),) and (a := self.expect_literal("/")):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location("/ may appear only once", a)
        self._reset(mark)
        a = None
        if (
            self.call_invalid_rules
            and (self.slash_no_default(),)
            and (self._loop0_187(),)
            and (self.invalid_parameters_helper())
            and (a := self.param_no_default())
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "parameter without a default follows parameter with a default", a
            )
        self._reset(mark)
        a = None
        if (
            (self._loop0_188(),)
            and (a := self.expect_literal("("))
            and (self._loop1_189())
            and (self.expect_literal(","),)
            and (b := self.expect_literal(")"))
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.raise_syntax_error_known_range(
                "Function parameters cannot be parenthesized", a, b
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (self._tmp_190(),)
            and (self._loop0_191(),)
            and (self.expect_literal("*"))
            and (self._tmp_192())
            and (self._loop0_193(),)
            and (a := self.expect_literal("/"))
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location("/ must be ahead of *", a)
        self._reset(mark)
        a = None
        if (self._loop1_194()) and (self.expect_literal("/")) and (a := self.expect_literal("*")):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location("expected comma between / and *", a)
        self._reset(mark)
        a = None
        return None

    def invalid_default(self) -> None:
        # invalid_default: '=' &(')' | ',')
        mark = self._mark()
        if (a := self.expect_literal("=")) and (
            self.positive_lookahead(
                self._tmp_195,
            )
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location("expected default value expression", a)
        self._reset(mark)
        a = None
        return None

    def invalid_star_etc(self) -> None:
        # invalid_star_etc: '*' (')' | (',' (')' | '**'))) | '*' param '=' | '*' (param_no_default | ',') param_maybe_default* '*' (param_no_default | ',')
        mark = self._mark()
        if (a := self.expect_literal("*")) and (self._tmp_196()):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location("named arguments must follow bare *", a)
        self._reset(mark)
        a = None
        if (self.expect_literal("*")) and (self.param()) and (a := self.expect_literal("=")):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "var-positional argument cannot have default value", a
            )
        self._reset(mark)
        a = None
        if (
            (self.expect_literal("*"))
            and (self._tmp_197())
            and (self._loop0_198(),)
            and (a := self.expect_literal("*"))
            and (self._tmp_199())
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location("* argument may appear only once", a)
        self._reset(mark)
        a = None
        return None

    def invalid_star(self) -> None:
        # invalid_star: '*'
        mark = self._mark()
        if self.expect_literal("*"):
            return self.raise_syntax_error("Invalid star expression")
        self._reset(mark)
        return None

    def invalid_kwds(self) -> None:
        # invalid_kwds: '**' param '=' | '**' param ',' param | '**' param ',' ('*' | '**' | '/')
        mark = self._mark()
        if (self.expect_literal("**")) and (self.param()) and (a := self.expect_literal("=")):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "var-keyword argument cannot have default value", a
            )
        self._reset(mark)
        a = None
        if (
            (self.expect_literal("**"))
            and (self.param())
            and (self.expect_literal(","))
            and (a := self.param())
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "arguments cannot follow var-keyword argument", a
            )
        self._reset(mark)
        a = None
        if (
            (self.expect_literal("**"))
            and (self.param())
            and (self.expect_literal(","))
            and (a := self._tmp_200())
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "arguments cannot follow var-keyword argument", a
            )
        self._reset(mark)
        a = None
        return None

    def invalid_parameters_helper(self) -> Optional:
        # invalid_parameters_helper: slash_with_default | param_with_default+
        mark = self._mark()
        if a := self.slash_with_default():
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if a := self._loop1_201():
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def invalid_lambda_parameters(self) -> None:
        # invalid_lambda_parameters: "/" ',' | (lambda_slash_no_default | lambda_slash_with_default) lambda_param_maybe_default* '/' | lambda_slash_no_default? lambda_param_no_default* invalid_lambda_parameters_helper lambda_param_no_default | lambda_param_no_default* '(' ','.lambda_param+ ','? ')' | [(lambda_slash_no_default | lambda_slash_with_default)] lambda_param_maybe_default* '*' (',' | lambda_param_no_default) lambda_param_maybe_default* '/' | lambda_param_maybe_default+ '/' '*'
        mark = self._mark()
        if (a := self.expect_literal("/")) and (self.expect_literal(",")):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "at least one argument must precede /", a
            )
        self._reset(mark)
        a = None
        if (self._tmp_202()) and (self._loop0_203(),) and (a := self.expect_literal("/")):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location("/ may appear only once", a)
        self._reset(mark)
        a = None
        if (
            self.call_invalid_rules
            and (self.lambda_slash_no_default(),)
            and (self._loop0_204(),)
            and (self.invalid_lambda_parameters_helper())
            and (a := self.lambda_param_no_default())
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "parameter without a default follows parameter with a default", a
            )
        self._reset(mark)
        a = None
        if (
            (self._loop0_205(),)
            and (a := self.expect_literal("("))
            and (self._gather_206())
            and (self.expect_literal(","),)
            and (b := self.expect_literal(")"))
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.raise_syntax_error_known_range(
                "Lambda expression parameters cannot be parenthesized", a, b
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (self._tmp_208(),)
            and (self._loop0_209(),)
            and (self.expect_literal("*"))
            and (self._tmp_210())
            and (self._loop0_211(),)
            and (a := self.expect_literal("/"))
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location("/ must be ahead of *", a)
        self._reset(mark)
        a = None
        if (self._loop1_212()) and (self.expect_literal("/")) and (a := self.expect_literal("*")):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location("expected comma between / and *", a)
        self._reset(mark)
        a = None
        return None

    def invalid_lambda_parameters_helper(self) -> Optional:
        # invalid_lambda_parameters_helper: lambda_slash_with_default | lambda_param_with_default+
        mark = self._mark()
        if a := self.lambda_slash_with_default():
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        if a := self._loop1_213():
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def invalid_lambda_star_etc(self) -> None:
        # invalid_lambda_star_etc: '*' (':' | ',' (':' | '**')) | '*' lambda_param '=' | '*' (lambda_param_no_default | ',') lambda_param_maybe_default* '*' (lambda_param_no_default | ',')
        mark = self._mark()
        if (self.expect_literal("*")) and (self._tmp_214()):
            return self.raise_syntax_error("named arguments must follow bare *")
        self._reset(mark)
        if (
            (self.expect_literal("*"))
            and (self.lambda_param())
            and (a := self.expect_literal("="))
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "var-positional argument cannot have default value", a
            )
        self._reset(mark)
        a = None
        if (
            (self.expect_literal("*"))
            and (self._tmp_215())
            and (self._loop0_216(),)
            and (a := self.expect_literal("*"))
            and (self._tmp_217())
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location("* argument may appear only once", a)
        self._reset(mark)
        a = None
        return None

    def invalid_lambda_kwds(self) -> None:
        # invalid_lambda_kwds: '**' lambda_param '=' | '**' lambda_param ',' lambda_param | '**' lambda_param ',' ('*' | '**' | '/')
        mark = self._mark()
        if (
            (self.expect_literal("**"))
            and (self.lambda_param())
            and (a := self.expect_literal("="))
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "var-keyword argument cannot have default value", a
            )
        self._reset(mark)
        a = None
        if (
            (self.expect_literal("**"))
            and (self.lambda_param())
            and (self.expect_literal(","))
            and (a := self.lambda_param())
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "arguments cannot follow var-keyword argument", a
            )
        self._reset(mark)
        a = None
        if (
            (self.expect_literal("**"))
            and (self.lambda_param())
            and (self.expect_literal(","))
            and (a := self._tmp_218())
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "arguments cannot follow var-keyword argument", a
            )
        self._reset(mark)
        a = None
        return None

    def invalid_with_item(self) -> None:
        # invalid_with_item: expression 'as' expression &(',' | ')' | ':')
        mark = self._mark()
        if (
            (self.expression())
            and (self.expect_literal("as"))
            and (a := self.expression())
            and (
                self.positive_lookahead(
                    self._tmp_219,
                )
            )
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_invalid_target(Target.STAR_TARGETS, a)
        self._reset(mark)
        a = None
        return None

    def invalid_for_target(self) -> None:
        # invalid_for_target: 'async'? 'for' star_expressions
        mark = self._mark()
        if (
            (self.expect_literal("async"),)
            and (self.expect_literal("for"))
            and (a := self.star_expressions())
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_invalid_target(Target.FOR_TARGETS, a)
        self._reset(mark)
        a = None
        return None

    def invalid_group(self) -> None:
        # invalid_group: '(' starred_expression ')' | '(' '**' expression ')'
        mark = self._mark()
        if (
            (self.expect_literal("("))
            and (a := self.starred_expression())
            and (self.expect_literal(")"))
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location("cannot use starred expression here", a)
        self._reset(mark)
        a = None
        if (
            (self.expect_literal("("))
            and (a := self.expect_literal("**"))
            and (self.expression())
            and (self.expect_literal(")"))
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "cannot use double starred expression here", a
            )
        self._reset(mark)
        a = None
        return None

    def invalid_import(self) -> None:
        # invalid_import: 'import' ','.dotted_name+ 'from' dotted_name
        mark = self._mark()
        if (
            (a := self.expect_literal("import"))
            and (self._gather_220())
            and (self.expect_literal("from"))
            and (self.dotted_name())
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_starting_from(
                "Did you mean to use 'from ... import ...' instead?", a
            )
        self._reset(mark)
        a = None
        return None

    def invalid_import_from_targets(self) -> None:
        # invalid_import_from_targets: import_from_as_names ',' NEWLINE
        mark = self._mark()
        if (
            (self.import_from_as_names())
            and (self.expect_literal(","))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
        ):
            return self.raise_syntax_error(
                "trailing comma not allowed without surrounding parentheses"
            )
        self._reset(mark)
        return None

    def invalid_with_stmt(self) -> None:
        # invalid_with_stmt: 'async'? 'with' ','.(expression ['as' star_target])+ &&':' | 'async'? 'with' '(' ','.(expressions ['as' star_target])+ ','? ')' &&':'
        mark = self._mark()
        if (
            (self.expect_literal("async"),)
            and (self.expect_literal("with"))
            and (self._gather_222())
            and (self.expect_forced(self.expect_literal(":"), "':'"))
        ):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expect_literal("async"),)
            and (self.expect_literal("with"))
            and (self.expect_literal("("))
            and (self._gather_224())
            and (self.expect_literal(","),)
            and (self.expect_literal(")"))
            and (self.expect_forced(self.expect_literal(":"), "':'"))
        ):
            return None  # pragma: no cover
        self._reset(mark)
        return None

    def invalid_with_stmt_indent(self) -> None:
        # invalid_with_stmt_indent: 'async'? 'with' ','.(expression ['as' star_target])+ ':' NEWLINE !INDENT | 'async'? 'with' '(' ','.(expressions ['as' star_target])+ ','? ')' ':' NEWLINE !INDENT
        mark = self._mark()
        if (
            (self.expect_literal("async"),)
            and (a := self.expect_literal("with"))
            and (self._gather_226())
            and (self.expect_literal(":"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.negative_lookahead(self.expect_type, tokenize.Tokens.INDENT))
        ):
            a = Codon.unwrap(a)
            return self.raise_indentation_error(
                f"expected an indented block after 'with' statement on line {a.start[0]}"
            )
        self._reset(mark)
        a = None
        if (
            (self.expect_literal("async"),)
            and (a := self.expect_literal("with"))
            and (self.expect_literal("("))
            and (self._gather_228())
            and (self.expect_literal(","),)
            and (self.expect_literal(")"))
            and (self.expect_literal(":"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.negative_lookahead(self.expect_type, tokenize.Tokens.INDENT))
        ):
            a = Codon.unwrap(a)
            return self.raise_indentation_error(
                f"expected an indented block after 'with' statement on line {a.start[0]}"
            )
        self._reset(mark)
        a = None
        return None

    def invalid_try_stmt(self) -> None:
        # invalid_try_stmt: 'try' ':' NEWLINE !INDENT | 'try' ':' block !('except' | 'finally') | 'try' ':' block* except_block+ 'except' '*' expression ['as' NAME] ':' | 'try' &&':' block except_star_block+ else_block? finally_block? | 'try' ':' block* except_star_block+ 'except' [expression ['as' NAME]] ':'
        mark = self._mark()
        if (
            (a := self.expect_literal("try"))
            and (self.expect_literal(":"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.negative_lookahead(self.expect_type, tokenize.Tokens.INDENT))
        ):
            a = Codon.unwrap(a)
            return self.raise_indentation_error(
                f"expected an indented block after 'try' statement on line {a.start[0]}",
            )
        self._reset(mark)
        a = None
        if (
            (self.expect_literal("try"))
            and (self.expect_literal(":"))
            and (self.block())
            and (
                self.negative_lookahead(
                    self._tmp_230,
                )
            )
        ):
            return self.raise_syntax_error("expected 'except' or 'finally' block")
        self._reset(mark)
        if (
            (self.expect_literal("try"))
            and (self.expect_literal(":"))
            and (self._loop0_231(),)
            and (self._loop1_232())
            and (a := self.expect_literal("except"))
            and (b := self.expect_literal("*"))
            and (self.expression())
            and (self._tmp_233(),)
            and (self.expect_literal(":"))
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.raise_syntax_error_known_range(
                "cannot have both 'except' and 'except*' on the same 'try'", a, b
            )
        self._reset(mark)
        a = None
        b = None
        if (
            (self.expect_literal("try"))
            and (self.expect_forced(self.expect_literal(":"), "':'"))
            and (self.block())
            and (self._loop1_234())
            and (self.else_block(),)
            and (self.finally_block(),)
        ):
            return self.raise_syntax_error_known_location("except* not yet supported", a)
        self._reset(mark)
        if (
            (self.expect_literal("try"))
            and (self.expect_literal(":"))
            and (self._loop0_235(),)
            and (self._loop1_236())
            and (a := self.expect_literal("except"))
            and (self._tmp_237(),)
            and (self.expect_literal(":"))
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "cannot have both 'except' and 'except*' on the same 'try'", a
            )
        self._reset(mark)
        a = None
        return None

    def invalid_except_stmt(self) -> None:
        # invalid_except_stmt: 'except' '*'? expression ',' expressions ['as' NAME] ':' | 'except' '*'? expression ['as' NAME] NEWLINE | 'except' '*'? NEWLINE | 'except' '*' (NEWLINE | ':')
        mark = self._mark()
        if (
            (self.expect_literal("except"))
            and (self.expect_literal("*"),)
            and (a := self.expression())
            and (self.expect_literal(","))
            and (self.expressions())
            and (self._tmp_238(),)
            and (self.expect_literal(":"))
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_starting_from(
                "multiple exception types must be parenthesized", a
            )
        self._reset(mark)
        a = None
        if (
            (self.expect_literal("except"))
            and (self.expect_literal("*"),)
            and (self.expression())
            and (self._tmp_239(),)
            and (self.expect_type(tokenize.Tokens.NEWLINE))
        ):
            return self.raise_syntax_error("expected ':'")
        self._reset(mark)
        if (
            (self.expect_literal("except"))
            and (self.expect_literal("*"),)
            and (self.expect_type(tokenize.Tokens.NEWLINE))
        ):
            return self.raise_syntax_error("expected ':'")
        self._reset(mark)
        if (self.expect_literal("except")) and (self.expect_literal("*")) and (self._tmp_240()):
            return self.raise_syntax_error("expected one or more exception types")
        self._reset(mark)
        return None

    def invalid_finally_stmt(self) -> None:
        # invalid_finally_stmt: 'finally' ':' NEWLINE !INDENT
        mark = self._mark()
        if (
            (a := self.expect_literal("finally"))
            and (self.expect_literal(":"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.negative_lookahead(self.expect_type, tokenize.Tokens.INDENT))
        ):
            a = Codon.unwrap(a)
            return self.raise_indentation_error(
                f"expected an indented block after 'finally' statement on line {a.start[0]}"
            )
        self._reset(mark)
        a = None
        return None

    def invalid_except_stmt_indent(self) -> None:
        # invalid_except_stmt_indent: 'except' expression ['as' NAME] ':' NEWLINE !INDENT | 'except' ':' NEWLINE !INDENT
        mark = self._mark()
        if (
            (a := self.expect_literal("except"))
            and (self.expression())
            and (self._tmp_241(),)
            and (self.expect_literal(":"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.negative_lookahead(self.expect_type, tokenize.Tokens.INDENT))
        ):
            a = Codon.unwrap(a)
            return self.raise_indentation_error(
                f"expected an indented block after 'except' statement on line {a.start[0]}"
            )
        self._reset(mark)
        a = None
        if (
            (a := self.expect_literal("except"))
            and (self.expect_literal(":"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.negative_lookahead(self.expect_type, tokenize.Tokens.INDENT))
        ):
            a = Codon.unwrap(a)
            return self.raise_indentation_error(
                f"expected an indented block after 'except' statement on line {a.start[0]}"
            )
        self._reset(mark)
        a = None
        return None

    def invalid_except_star_stmt_indent(self) -> None:
        # invalid_except_star_stmt_indent: 'except' '*' expression ['as' NAME] ':' NEWLINE !INDENT
        mark = self._mark()
        if (
            (a := self.expect_literal("except"))
            and (self.expect_literal("*"))
            and (self.expression())
            and (self._tmp_242(),)
            and (self.expect_literal(":"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.negative_lookahead(self.expect_type, tokenize.Tokens.INDENT))
        ):
            a = Codon.unwrap(a)
            return self.raise_indentation_error(
                f"expected an indented block after 'except*' statement on line {a.start[0]}"
            )
        self._reset(mark)
        a = None
        return None

    def invalid_match_stmt(self) -> None:
        # invalid_match_stmt: "match" subject_expr !':' | "match" subject_expr ':' NEWLINE !INDENT
        mark = self._mark()
        if (
            (self.expect_literal("match"))
            and (self.subject_expr())
            and (self.negative_lookahead(self.expect_literal, ":"))
        ):
            return self.raise_syntax_error("expected ':'")
        self._reset(mark)
        if (
            (a := self.expect_literal("match"))
            and (self.subject_expr())
            and (self.expect_literal(":"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.negative_lookahead(self.expect_type, tokenize.Tokens.INDENT))
        ):
            a = Codon.unwrap(a)
            return self.raise_indentation_error(
                f"expected an indented block after 'match' statement on line {a.start[0]}"
            )
        self._reset(mark)
        a = None
        return None

    def invalid_case_block(self) -> None:
        # invalid_case_block: "case" patterns guard? !':' | "case" patterns guard? ':' NEWLINE !INDENT
        mark = self._mark()
        if (
            (self.expect_literal("case"))
            and (self.patterns())
            and (self.guard(),)
            and (self.negative_lookahead(self.expect_literal, ":"))
        ):
            return self.raise_syntax_error("expected ':'")
        self._reset(mark)
        if (
            (a := self.expect_literal("case"))
            and (self.patterns())
            and (self.guard(),)
            and (self.expect_literal(":"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.negative_lookahead(self.expect_type, tokenize.Tokens.INDENT))
        ):
            a = Codon.unwrap(a)
            return self.raise_indentation_error(
                f"expected an indented block after 'case' statement on line {a.start[0]}"
            )
        self._reset(mark)
        a = None
        return None

    def invalid_as_pattern(self) -> None:
        # invalid_as_pattern: or_pattern 'as' "_" | or_pattern 'as' !NAME expression
        mark = self._mark()
        if (self.or_pattern()) and (self.expect_literal("as")) and (a := self.expect_literal("_")):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location("cannot use '_' as a target", a)
        self._reset(mark)
        a = None
        if (
            (self.or_pattern())
            and (self.expect_literal("as"))
            and (
                self.negative_lookahead(
                    self.name,
                )
            )
            and (a := self.expression())
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location("invalid pattern target", a)
        self._reset(mark)
        a = None
        return None

    def invalid_class_pattern(self) -> None:
        # invalid_class_pattern: name_or_attr '(' invalid_class_argument_pattern
        mark = self._mark()
        if (
            self.call_invalid_rules
            and (self.name_or_attr())
            and (self.expect_literal("("))
            and (a := self.invalid_class_argument_pattern())
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_range(
                "positional patterns follow keyword patterns", a[0], a[-1]
            )
        self._reset(mark)
        a = None
        return None

    def invalid_class_argument_pattern(self) -> Optional[list]:
        # invalid_class_argument_pattern: [positional_patterns ','] keyword_patterns ',' positional_patterns
        mark = self._mark()
        if (
            (self._tmp_243(),)
            and (self.keyword_patterns())
            and (self.expect_literal(","))
            and (a := self.positional_patterns())
        ):
            a = Codon.unwrap(a)
            return a
        self._reset(mark)
        a = None
        return None

    def invalid_if_stmt(self) -> None:
        # invalid_if_stmt: 'if' named_expression NEWLINE | 'if' named_expression ':' NEWLINE !INDENT
        mark = self._mark()
        if (
            (self.expect_literal("if"))
            and (self.named_expression())
            and (self.expect_type(tokenize.Tokens.NEWLINE))
        ):
            return self.raise_syntax_error("expected ':'")
        self._reset(mark)
        if (
            (a := self.expect_literal("if"))
            and (a_1 := self.named_expression())
            and (self.expect_literal(":"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.negative_lookahead(self.expect_type, tokenize.Tokens.INDENT))
        ):
            a = Codon.unwrap(a)
            a_1 = Codon.unwrap(a_1)
            return self.raise_indentation_error(
                f"expected an indented block after 'if' statement on line {a.start[0]}"
            )
        self._reset(mark)
        a = None
        a_1 = None
        return None

    def invalid_elif_stmt(self) -> None:
        # invalid_elif_stmt: 'elif' named_expression NEWLINE | 'elif' named_expression ':' NEWLINE !INDENT
        mark = self._mark()
        if (
            (self.expect_literal("elif"))
            and (self.named_expression())
            and (self.expect_type(tokenize.Tokens.NEWLINE))
        ):
            return self.raise_syntax_error("expected ':'")
        self._reset(mark)
        if (
            (a := self.expect_literal("elif"))
            and (self.named_expression())
            and (self.expect_literal(":"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.negative_lookahead(self.expect_type, tokenize.Tokens.INDENT))
        ):
            a = Codon.unwrap(a)
            return self.raise_indentation_error(
                f"expected an indented block after 'elif' statement on line {a.start[0]}"
            )
        self._reset(mark)
        a = None
        return None

    def invalid_else_stmt(self) -> None:
        # invalid_else_stmt: 'else' ':' NEWLINE !INDENT
        mark = self._mark()
        if (
            (a := self.expect_literal("else"))
            and (self.expect_literal(":"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.negative_lookahead(self.expect_type, tokenize.Tokens.INDENT))
        ):
            a = Codon.unwrap(a)
            return self.raise_indentation_error(
                f"expected an indented block after 'else' statement on line {a.start[0]}"
            )
        self._reset(mark)
        a = None
        return None

    def invalid_while_stmt(self) -> None:
        # invalid_while_stmt: 'while' named_expression NEWLINE | 'while' named_expression ':' NEWLINE !INDENT
        mark = self._mark()
        if (
            (self.expect_literal("while"))
            and (self.named_expression())
            and (self.expect_type(tokenize.Tokens.NEWLINE))
        ):
            return self.raise_syntax_error("expected ':'")
        self._reset(mark)
        if (
            (a := self.expect_literal("while"))
            and (self.named_expression())
            and (self.expect_literal(":"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.negative_lookahead(self.expect_type, tokenize.Tokens.INDENT))
        ):
            a = Codon.unwrap(a)
            return self.raise_indentation_error(
                f"expected an indented block after 'while' statement on line {a.start[0]}"
            )
        self._reset(mark)
        a = None
        return None

    def invalid_for_stmt(self) -> None:
        # invalid_for_stmt: ASYNC? 'for' star_targets 'in' star_expressions NEWLINE | 'async'? 'for' star_targets 'in' star_expressions ':' NEWLINE !INDENT
        mark = self._mark()
        if (
            (self.expect_type(tokenize.Tokens.ASYNC),)
            and (self.expect_literal("for"))
            and (self.star_targets())
            and (self.expect_literal("in"))
            and (self.star_expressions())
            and (self.expect_type(tokenize.Tokens.NEWLINE))
        ):
            return self.raise_syntax_error("expected ':'")
        self._reset(mark)
        if (
            (self.expect_literal("async"),)
            and (a := self.expect_literal("for"))
            and (self.star_targets())
            and (self.expect_literal("in"))
            and (self.star_expressions())
            and (self.expect_literal(":"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.negative_lookahead(self.expect_type, tokenize.Tokens.INDENT))
        ):
            a = Codon.unwrap(a)
            return self.raise_indentation_error(
                f"expected an indented block after 'for' statement on line {a.start[0]}"
            )
        self._reset(mark)
        a = None
        return None

    def invalid_def_raw(self) -> None:
        # invalid_def_raw: 'async'? 'def' NAME type_params? '(' params? ')' ['->' expression] ':' NEWLINE !INDENT
        mark = self._mark()
        if (
            (self.expect_literal("async"),)
            and (a := self.expect_literal("def"))
            and (self.name())
            and (self.type_params(),)
            and (self.expect_literal("("))
            and (self.params(),)
            and (self.expect_literal(")"))
            and (self._tmp_244(),)
            and (self.expect_literal(":"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.negative_lookahead(self.expect_type, tokenize.Tokens.INDENT))
        ):
            a = Codon.unwrap(a)
            return self.raise_indentation_error(
                f"expected an indented block after function definition on line {a.start[0]}"
            )
        self._reset(mark)
        a = None
        return None

    def invalid_class_def_raw(self) -> None:
        # invalid_class_def_raw: 'class' NAME type_params? ['(' arguments? ')'] NEWLINE | 'class' NAME type_params? ['(' arguments? ')'] ':' NEWLINE !INDENT
        mark = self._mark()
        if (
            (self.expect_literal("class"))
            and (self.name())
            and (self.type_params(),)
            and (self._tmp_245(),)
            and (self.expect_type(tokenize.Tokens.NEWLINE))
        ):
            return self.raise_syntax_error("expected ':'")
        self._reset(mark)
        if (
            (a := self.expect_literal("class"))
            and (self.name())
            and (self.type_params(),)
            and (self._tmp_246(),)
            and (self.expect_literal(":"))
            and (self.expect_type(tokenize.Tokens.NEWLINE))
            and (self.negative_lookahead(self.expect_type, tokenize.Tokens.INDENT))
        ):
            a = Codon.unwrap(a)
            return self.raise_indentation_error(
                f"expected an indented block after class definition on line {a.start[0]}"
            )
        self._reset(mark)
        a = None
        return None

    def invalid_double_starred_kvpairs(self) -> None:
        # invalid_double_starred_kvpairs: ','.double_starred_kvpair+ ',' invalid_kvpair | expression ':' '*' bitwise_or | expression ':' &('}' | ',')
        mark = self._mark()
        if (
            self.call_invalid_rules
            and (self._gather_247())
            and (self.expect_literal(","))
            and (self.invalid_kvpair())
        ):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expression())
            and (self.expect_literal(":"))
            and (a := self.expect_literal("*"))
            and (self.bitwise_or())
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_starting_from(
                "cannot use a starred expression in a dictionary value", a
            )
        self._reset(mark)
        a = None
        if (
            (self.expression())
            and (a := self.expect_literal(":"))
            and (
                self.positive_lookahead(
                    self._tmp_249,
                )
            )
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "expression expected after dictionary key and ':'", a
            )
        self._reset(mark)
        a = None
        return None

    def invalid_kvpair(self) -> None:
        # invalid_kvpair: expression !(':') | expression ':' '*' bitwise_or | expression ':' &('}' | ',') | expression ':'
        mark = self._mark()
        if (a := self.expression()) and (self.negative_lookahead(self.expect_literal, ":")):
            a = Codon.unwrap(a)
            return self.raise_raw_syntax_error(
                "':' expected after dictionary key",
                (a.lineno, a.col_offset),
                (a.end_lineno, a.end_col_offset),
            )
        self._reset(mark)
        a = None
        if (
            (self.expression())
            and (self.expect_literal(":"))
            and (a := self.expect_literal("*"))
            and (self.bitwise_or())
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_starting_from(
                "cannot use a starred expression in a dictionary value", a
            )
        self._reset(mark)
        a = None
        if (
            (self.expression())
            and (a := self.expect_literal(":"))
            and (
                self.positive_lookahead(
                    self._tmp_250,
                )
            )
        ):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "expression expected after dictionary key and ':'", a
            )
        self._reset(mark)
        a = None
        if (self.expression()) and (a := self.expect_literal(":")):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "expression expected after dictionary key and ':'", a
            )
        self._reset(mark)
        a = None
        return None

    def invalid_starred_expression(self) -> None:
        # invalid_starred_expression: '*' expression '=' expression
        mark = self._mark()
        if (
            (a := self.expect_literal("*"))
            and (self.expression())
            and (self.expect_literal("="))
            and (b := self.expression())
        ):
            a = Codon.unwrap(a)
            b = Codon.unwrap(b)
            return self.raise_syntax_error_known_range(
                "cannot assign to iterable argument unpacking", a, b
            )
        self._reset(mark)
        a = None
        b = None
        return None

    def invalid_replacement_field(self) -> None:
        # invalid_replacement_field: '{' '=' | '{' '!' | '{' ':' | '{' '}' | '{' !(yield_expr | star_expressions) | '{' (yield_expr | star_expressions) !('=' | '!' | ':' | '}') | '{' (yield_expr | star_expressions) '=' !('!' | ':' | '}') | '{' (yield_expr | star_expressions) '='? invalid_conversion_character | '{' (yield_expr | star_expressions) '='? ['!' NAME] !(':' | '}') | '{' (yield_expr | star_expressions) '='? ['!' NAME] ':' fstring_format_spec* !'}' | '{' (yield_expr | star_expressions) '='? ['!' NAME] !'}'
        mark = self._mark()
        if (self.expect_literal("{")) and (a := self.expect_literal("=")):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "f-string: valid expression required before '='", a
            )
        self._reset(mark)
        a = None
        if (self.expect_literal("{")) and (a := self.expect_literal("!")):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "f-string: valid expression required before '!'", a
            )
        self._reset(mark)
        a = None
        if (self.expect_literal("{")) and (a := self.expect_literal(":")):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "f-string: valid expression required before ':'", a
            )
        self._reset(mark)
        a = None
        if (self.expect_literal("{")) and (a := self.expect_literal("}")):
            a = Codon.unwrap(a)
            return self.raise_syntax_error_known_location(
                "f-string: valid expression required before '}'", a
            )
        self._reset(mark)
        a = None
        if (self.expect_literal("{")) and (
            self.negative_lookahead(
                self._tmp_251,
            )
        ):
            return self.raise_syntax_error_on_next_token(
                "f-string: expecting a valid expression after '{'"
            )
        self._reset(mark)
        if (
            (self.expect_literal("{"))
            and (self._tmp_252())
            and (
                self.negative_lookahead(
                    self._tmp_253,
                )
            )
        ):
            return self.raise_syntax_error_on_next_token(
                "f-string: expecting '=', or '!', or ':', or '}'"
            )
        self._reset(mark)
        if (
            (self.expect_literal("{"))
            and (self._tmp_254())
            and (self.expect_literal("="))
            and (
                self.negative_lookahead(
                    self._tmp_255,
                )
            )
        ):
            return self.raise_syntax_error_on_next_token("f-string: expecting '!', or ':', or '}'")
        self._reset(mark)
        if (
            self.call_invalid_rules
            and (self.expect_literal("{"))
            and (self._tmp_256())
            and (self.expect_literal("="),)
            and (self.invalid_conversion_character())
        ):
            return None  # pragma: no cover
        self._reset(mark)
        if (
            (self.expect_literal("{"))
            and (self._tmp_257())
            and (self.expect_literal("="),)
            and (self._tmp_258(),)
            and (
                self.negative_lookahead(
                    self._tmp_259,
                )
            )
        ):
            return self.raise_syntax_error_on_next_token("f-string: expecting ':' or '}'")
        self._reset(mark)
        if (
            (self.expect_literal("{"))
            and (self._tmp_260())
            and (self.expect_literal("="),)
            and (self._tmp_261(),)
            and (self.expect_literal(":"))
            and (self._loop0_262(),)
            and (self.negative_lookahead(self.expect_literal, "}"))
        ):
            return self.raise_syntax_error_on_next_token(
                "f-string: expecting '}', or format specs"
            )
        self._reset(mark)
        if (
            (self.expect_literal("{"))
            and (self._tmp_263())
            and (self.expect_literal("="),)
            and (self._tmp_264(),)
            and (self.negative_lookahead(self.expect_literal, "}"))
        ):
            return self.raise_syntax_error_on_next_token("f-string: expecting '}'")
        self._reset(mark)
        return None

    def invalid_conversion_character(self) -> None:
        # invalid_conversion_character: '!' &(':' | '}') | '!' !NAME
        mark = self._mark()
        if (self.expect_literal("!")) and (
            self.positive_lookahead(
                self._tmp_265,
            )
        ):
            return self.raise_syntax_error_on_next_token("f-string: missing conversion character")
        self._reset(mark)
        if (self.expect_literal("!")) and (
            self.negative_lookahead(
                self.name,
            )
        ):
            return self.raise_syntax_error_on_next_token("f-string: invalid conversion character")
        self._reset(mark)
        return None

    def _loop0_1(self) -> List:
        # _loop0_1: fstring_mid
        mark = self._mark()
        children = []
        while fstring_mid_ := self.fstring_mid():
            fstring_mid = Codon.unwrap(fstring_mid_)
            children.append(fstring_mid)
            mark = self._mark()
        self._reset(mark)
        fstring_mid = None
        return children

    def _loop1_2(self) -> List:
        # _loop1_2: statement
        mark = self._mark()
        children = []
        while statement_ := self.statement():
            statement = Codon.unwrap(statement_)
            children.append(statement)
            mark = self._mark()
        self._reset(mark)
        statement = None
        return children

    def _loop0_4(self) -> List:
        # _loop0_4: ';' simple_stmt
        mark = self._mark()
        children = []
        while (self.expect_literal(";")) and (elem_ := self.simple_stmt()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_3(self) -> Optional:
        # _gather_3: simple_stmt _loop0_4
        mark = self._mark()
        if (elem := self.simple_stmt()) is not None and (seq := self._loop0_4()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _tmp_5(self) -> Optional:
        # _tmp_5: "print" !'('
        mark = self._mark()
        if (literal := self.expect_literal("print")) and (
            self.negative_lookahead(self.expect_literal, "(")
        ):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_6(self) -> Optional:
        # _tmp_6: 'import' | 'from'
        mark = self._mark()
        if literal := self.expect_literal("import"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("from"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_7(self) -> Optional:
        # _tmp_7: 'def' | '@' | 'async'
        mark = self._mark()
        if literal := self.expect_literal("def"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("@"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("async"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_8(self) -> Optional:
        # _tmp_8: 'class' | '@'
        mark = self._mark()
        if literal := self.expect_literal("class"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("@"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_9(self) -> Optional:
        # _tmp_9: 'with' | 'async'
        mark = self._mark()
        if literal := self.expect_literal("with"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("async"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_10(self) -> Optional:
        # _tmp_10: 'for' | '@' | 'async'
        mark = self._mark()
        if literal := self.expect_literal("for"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("@"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("async"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_11(self) -> Optional:
        # _tmp_11: '=' annotated_rhs
        mark = self._mark()
        if (self.expect_literal("=")) and (d := self.annotated_rhs()):
            d = Codon.unwrap(d)
            return d
        self._reset(mark)
        d = None
        return None

    def _tmp_12(self) -> Optional:
        # _tmp_12: '(' single_target ')' | single_subscript_attribute_target
        mark = self._mark()
        if (
            (self.expect_literal("("))
            and (b := self.single_target())
            and (self.expect_literal(")"))
        ):
            b = Codon.unwrap(b)
            return b
        self._reset(mark)
        b = None
        if single_subscript_attribute_target := self.single_subscript_attribute_target():
            single_subscript_attribute_target = Codon.unwrap(single_subscript_attribute_target)
            return single_subscript_attribute_target
        self._reset(mark)
        single_subscript_attribute_target = None
        return None

    def _tmp_13(self) -> Optional:
        # _tmp_13: '=' annotated_rhs
        mark = self._mark()
        if (self.expect_literal("=")) and (d := self.annotated_rhs()):
            d = Codon.unwrap(d)
            return d
        self._reset(mark)
        d = None
        return None

    def _loop1_14(self) -> List:
        # _loop1_14: (star_targets '=')
        mark = self._mark()
        children = []
        while _tmp_266_ := self._tmp_266():
            _tmp_266 = Codon.unwrap(_tmp_266_)
            children.append(_tmp_266)
            mark = self._mark()
        self._reset(mark)
        _tmp_266 = None
        return children

    def _tmp_15(self) -> Optional:
        # _tmp_15: yield_expr | star_expressions
        mark = self._mark()
        if yield_expr := self.yield_expr():
            yield_expr = Codon.unwrap(yield_expr)
            return yield_expr
        self._reset(mark)
        yield_expr = None
        if star_expressions := self.star_expressions():
            star_expressions = Codon.unwrap(star_expressions)
            return star_expressions
        self._reset(mark)
        star_expressions = None
        return None

    def _tmp_16(self) -> Optional:
        # _tmp_16: yield_expr | star_expressions
        mark = self._mark()
        if yield_expr := self.yield_expr():
            yield_expr = Codon.unwrap(yield_expr)
            return yield_expr
        self._reset(mark)
        yield_expr = None
        if star_expressions := self.star_expressions():
            star_expressions = Codon.unwrap(star_expressions)
            return star_expressions
        self._reset(mark)
        star_expressions = None
        return None

    def _tmp_17(self) -> Optional:
        # _tmp_17: 'from' expression
        mark = self._mark()
        if (self.expect_literal("from")) and (z := self.expression()):
            z = Codon.unwrap(z)
            return z
        self._reset(mark)
        z = None
        return None

    def _loop0_19(self) -> List:
        # _loop0_19: ',' NAME
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.name()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_18(self) -> Optional:
        # _gather_18: NAME _loop0_19
        mark = self._mark()
        if (elem := self.name()) is not None and (seq := self._loop0_19()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop0_21(self) -> List:
        # _loop0_21: ',' NAME
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.name()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_20(self) -> Optional:
        # _gather_20: NAME _loop0_21
        mark = self._mark()
        if (elem := self.name()) is not None and (seq := self._loop0_21()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop0_23(self) -> List:
        # _loop0_23: ',' expression
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.expression()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_22(self) -> Optional:
        # _gather_22: expression _loop0_23
        mark = self._mark()
        if (elem := self.expression()) is not None and (seq := self._loop0_23()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _tmp_24(self) -> Optional:
        # _tmp_24: ';' | NEWLINE
        mark = self._mark()
        if literal := self.expect_literal(";"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if _newline := self.expect_type(tokenize.Tokens.NEWLINE):
            _newline = Codon.unwrap(_newline)
            return _newline
        self._reset(mark)
        _newline = None
        return None

    def _tmp_25(self) -> Optional:
        # _tmp_25: ',' expression
        mark = self._mark()
        if (self.expect_literal(",")) and (z := self.expression()):
            z = Codon.unwrap(z)
            return z
        self._reset(mark)
        z = None
        return None

    def _loop0_26(self) -> List:
        # _loop0_26: ('.' | '...')
        mark = self._mark()
        children = []
        while _tmp_267_ := self._tmp_267():
            _tmp_267 = Codon.unwrap(_tmp_267_)
            children.append(_tmp_267)
            mark = self._mark()
        self._reset(mark)
        _tmp_267 = None
        return children

    def _loop1_27(self) -> List:
        # _loop1_27: ('.' | '...')
        mark = self._mark()
        children = []
        while _tmp_268_ := self._tmp_268():
            _tmp_268 = Codon.unwrap(_tmp_268_)
            children.append(_tmp_268)
            mark = self._mark()
        self._reset(mark)
        _tmp_268 = None
        return children

    def _loop0_29(self) -> List:
        # _loop0_29: ',' import_from_as_name
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.import_from_as_name()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_28(self) -> Optional:
        # _gather_28: import_from_as_name _loop0_29
        mark = self._mark()
        if (elem := self.import_from_as_name()) is not None and (
            seq := self._loop0_29()
        ) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop0_30(self) -> List:
        # _loop0_30: import_params
        mark = self._mark()
        children = []
        while import_params_ := self.import_params():
            import_params = Codon.unwrap(import_params_)
            children.append(import_params)
            mark = self._mark()
        self._reset(mark)
        import_params = None
        return children

    def _tmp_31(self) -> Optional:
        # _tmp_31: 'as' NAME
        mark = self._mark()
        if (self.expect_literal("as")) and (z := self.name()):
            z = Codon.unwrap(z)
            return z.string
        self._reset(mark)
        z = None
        return None

    def _loop0_32(self) -> List:
        # _loop0_32: import_params
        mark = self._mark()
        children = []
        while import_params_ := self.import_params():
            import_params = Codon.unwrap(import_params_)
            children.append(import_params)
            mark = self._mark()
        self._reset(mark)
        import_params = None
        return children

    def _tmp_33(self) -> Optional:
        # _tmp_33: 'as' NAME
        mark = self._mark()
        if (self.expect_literal("as")) and (z := self.name()):
            z = Codon.unwrap(z)
            return z.string
        self._reset(mark)
        z = None
        return None

    def _tmp_34(self) -> Optional:
        # _tmp_34: 'as' NAME
        mark = self._mark()
        if (self.expect_literal("as")) and (z := self.name()):
            z = Codon.unwrap(z)
            return z.string
        self._reset(mark)
        z = None
        return None

    def _tmp_35(self) -> Optional:
        # _tmp_35: 'as' NAME
        mark = self._mark()
        if (self.expect_literal("as")) and (z := self.name()):
            z = Codon.unwrap(z)
            return z.string
        self._reset(mark)
        z = None
        return None

    def _loop0_37(self) -> List:
        # _loop0_37: ',' dotted_as_name
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.dotted_as_name()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_36(self) -> Optional:
        # _gather_36: dotted_as_name _loop0_37
        mark = self._mark()
        if (elem := self.dotted_as_name()) is not None and (seq := self._loop0_37()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _tmp_38(self) -> Optional:
        # _tmp_38: 'as' NAME
        mark = self._mark()
        if (self.expect_literal("as")) and (z := self.name()):
            z = Codon.unwrap(z)
            return z.string
        self._reset(mark)
        z = None
        return None

    def _loop1_39(self) -> List:
        # _loop1_39: decorator
        mark = self._mark()
        children = []
        while decorator_ := self.decorator():
            decorator = Codon.unwrap(decorator_)
            children.append(decorator)
            mark = self._mark()
        self._reset(mark)
        decorator = None
        return children

    def _tmp_40(self) -> Optional:
        # _tmp_40: '@' dec_maybe_call NEWLINE
        mark = self._mark()
        if (
            (self.expect_literal("@"))
            and (f := self.dec_maybe_call())
            and (self.expect_type(tokenize.Tokens.NEWLINE))
        ):
            f = Codon.unwrap(f)
            return f
        self._reset(mark)
        f = None
        return None

    def _tmp_41(self) -> Optional:
        # _tmp_41: '@' named_expression NEWLINE
        mark = self._mark()
        if (
            (self.expect_literal("@"))
            and (f := self.named_expression())
            and (self.expect_type(tokenize.Tokens.NEWLINE))
        ):
            f = Codon.unwrap(f)
            return f
        self._reset(mark)
        f = None
        return None

    def _tmp_42(self) -> Optional:
        # _tmp_42: '(' arguments? ')'
        mark = self._mark()
        if (self.expect_literal("(")) and (z := self.arguments(),) and (self.expect_literal(")")):
            return z
        self._reset(mark)
        z = None
        return None

    def _loop0_43(self) -> List:
        # _loop0_43: decorator_not_llvm
        mark = self._mark()
        children = []
        while decorator_not_llvm_ := self.decorator_not_llvm():
            decorator_not_llvm = Codon.unwrap(decorator_not_llvm_)
            children.append(decorator_not_llvm)
            mark = self._mark()
        self._reset(mark)
        decorator_not_llvm = None
        return children

    def _loop0_44(self) -> List:
        # _loop0_44: decorator
        mark = self._mark()
        children = []
        while decorator_ := self.decorator():
            decorator = Codon.unwrap(decorator_)
            children.append(decorator)
            mark = self._mark()
        self._reset(mark)
        decorator = None
        return children

    def _tmp_45(self) -> Optional:
        # _tmp_45: '->' expression
        mark = self._mark()
        if (self.expect_literal("->")) and (z := self.expression()):
            z = Codon.unwrap(z)
            return z
        self._reset(mark)
        z = None
        return None

    def _tmp_46(self) -> Optional:
        # _tmp_46: '->' expression
        mark = self._mark()
        if (self.expect_literal("->")) and (z := self.expression()):
            z = Codon.unwrap(z)
            return z
        self._reset(mark)
        z = None
        return None

    def _tmp_47(self) -> Optional:
        # _tmp_47: '@' "llvm"
        mark = self._mark()
        if (literal := self.expect_literal("@")) and (literal_1 := self.expect_literal("llvm")):
            literal = Codon.unwrap(literal)
            literal_1 = Codon.unwrap(literal_1)
            return [literal, literal_1]
        self._reset(mark)
        literal = None
        literal_1 = None
        return None

    def _tmp_48(self) -> Optional:
        # _tmp_48: '->' expression
        mark = self._mark()
        if (self.expect_literal("->")) and (z := self.expression()):
            z = Codon.unwrap(z)
            return z
        self._reset(mark)
        z = None
        return None

    def _loop1_49(self) -> List:
        # _loop1_49: llvm_line
        mark = self._mark()
        children = []
        while llvm_line_ := self.llvm_line():
            llvm_line = Codon.unwrap(llvm_line_)
            children.append(llvm_line)
            mark = self._mark()
        self._reset(mark)
        llvm_line = None
        return children

    def _loop0_50(self) -> List:
        # _loop0_50: ANY_BUT_NEWLINE
        mark = self._mark()
        children = []
        while any_but_newline_ := self.any_but_newline():
            any_but_newline = Codon.unwrap(any_but_newline_)
            children.append(any_but_newline)
            mark = self._mark()
        self._reset(mark)
        any_but_newline = None
        return children

    def _loop0_51(self) -> List:
        # _loop0_51: param_no_default
        mark = self._mark()
        children = []
        while param_no_default_ := self.param_no_default():
            param_no_default = Codon.unwrap(param_no_default_)
            children.append(param_no_default)
            mark = self._mark()
        self._reset(mark)
        param_no_default = None
        return children

    def _loop0_52(self) -> List:
        # _loop0_52: param_with_default
        mark = self._mark()
        children = []
        while param_with_default_ := self.param_with_default():
            param_with_default = Codon.unwrap(param_with_default_)
            children.append(param_with_default)
            mark = self._mark()
        self._reset(mark)
        param_with_default = None
        return children

    def _loop0_53(self) -> List:
        # _loop0_53: param_with_default
        mark = self._mark()
        children = []
        while param_with_default_ := self.param_with_default():
            param_with_default = Codon.unwrap(param_with_default_)
            children.append(param_with_default)
            mark = self._mark()
        self._reset(mark)
        param_with_default = None
        return children

    def _loop1_54(self) -> List:
        # _loop1_54: param_no_default
        mark = self._mark()
        children = []
        while param_no_default_ := self.param_no_default():
            param_no_default = Codon.unwrap(param_no_default_)
            children.append(param_no_default)
            mark = self._mark()
        self._reset(mark)
        param_no_default = None
        return children

    def _loop0_55(self) -> List:
        # _loop0_55: param_with_default
        mark = self._mark()
        children = []
        while param_with_default_ := self.param_with_default():
            param_with_default = Codon.unwrap(param_with_default_)
            children.append(param_with_default)
            mark = self._mark()
        self._reset(mark)
        param_with_default = None
        return children

    def _loop1_56(self) -> List:
        # _loop1_56: param_with_default
        mark = self._mark()
        children = []
        while param_with_default_ := self.param_with_default():
            param_with_default = Codon.unwrap(param_with_default_)
            children.append(param_with_default)
            mark = self._mark()
        self._reset(mark)
        param_with_default = None
        return children

    def _loop1_57(self) -> List:
        # _loop1_57: param_no_default
        mark = self._mark()
        children = []
        while param_no_default_ := self.param_no_default():
            param_no_default = Codon.unwrap(param_no_default_)
            children.append(param_no_default)
            mark = self._mark()
        self._reset(mark)
        param_no_default = None
        return children

    def _loop1_58(self) -> List:
        # _loop1_58: param_no_default
        mark = self._mark()
        children = []
        while param_no_default_ := self.param_no_default():
            param_no_default = Codon.unwrap(param_no_default_)
            children.append(param_no_default)
            mark = self._mark()
        self._reset(mark)
        param_no_default = None
        return children

    def _loop0_59(self) -> List:
        # _loop0_59: param_no_default
        mark = self._mark()
        children = []
        while param_no_default_ := self.param_no_default():
            param_no_default = Codon.unwrap(param_no_default_)
            children.append(param_no_default)
            mark = self._mark()
        self._reset(mark)
        param_no_default = None
        return children

    def _loop1_60(self) -> List:
        # _loop1_60: param_with_default
        mark = self._mark()
        children = []
        while param_with_default_ := self.param_with_default():
            param_with_default = Codon.unwrap(param_with_default_)
            children.append(param_with_default)
            mark = self._mark()
        self._reset(mark)
        param_with_default = None
        return children

    def _loop0_61(self) -> List:
        # _loop0_61: param_no_default
        mark = self._mark()
        children = []
        while param_no_default_ := self.param_no_default():
            param_no_default = Codon.unwrap(param_no_default_)
            children.append(param_no_default)
            mark = self._mark()
        self._reset(mark)
        param_no_default = None
        return children

    def _loop1_62(self) -> List:
        # _loop1_62: param_with_default
        mark = self._mark()
        children = []
        while param_with_default_ := self.param_with_default():
            param_with_default = Codon.unwrap(param_with_default_)
            children.append(param_with_default)
            mark = self._mark()
        self._reset(mark)
        param_with_default = None
        return children

    def _loop0_63(self) -> List:
        # _loop0_63: param_maybe_default
        mark = self._mark()
        children = []
        while param_maybe_default_ := self.param_maybe_default():
            param_maybe_default = Codon.unwrap(param_maybe_default_)
            children.append(param_maybe_default)
            mark = self._mark()
        self._reset(mark)
        param_maybe_default = None
        return children

    def _loop0_64(self) -> List:
        # _loop0_64: codon_type_param
        mark = self._mark()
        children = []
        while codon_type_param_ := self.codon_type_param():
            codon_type_param = Codon.unwrap(codon_type_param_)
            children.append(codon_type_param)
            mark = self._mark()
        self._reset(mark)
        codon_type_param = None
        return children

    def _loop0_65(self) -> List:
        # _loop0_65: param_maybe_default
        mark = self._mark()
        children = []
        while param_maybe_default_ := self.param_maybe_default():
            param_maybe_default = Codon.unwrap(param_maybe_default_)
            children.append(param_maybe_default)
            mark = self._mark()
        self._reset(mark)
        param_maybe_default = None
        return children

    def _loop0_66(self) -> List:
        # _loop0_66: codon_type_param
        mark = self._mark()
        children = []
        while codon_type_param_ := self.codon_type_param():
            codon_type_param = Codon.unwrap(codon_type_param_)
            children.append(codon_type_param)
            mark = self._mark()
        self._reset(mark)
        codon_type_param = None
        return children

    def _loop1_67(self) -> List:
        # _loop1_67: param_maybe_default
        mark = self._mark()
        children = []
        while param_maybe_default_ := self.param_maybe_default():
            param_maybe_default = Codon.unwrap(param_maybe_default_)
            children.append(param_maybe_default)
            mark = self._mark()
        self._reset(mark)
        param_maybe_default = None
        return children

    def _loop0_68(self) -> List:
        # _loop0_68: codon_type_param
        mark = self._mark()
        children = []
        while codon_type_param_ := self.codon_type_param():
            codon_type_param = Codon.unwrap(codon_type_param_)
            children.append(codon_type_param)
            mark = self._mark()
        self._reset(mark)
        codon_type_param = None
        return children

    def _loop0_69(self) -> List:
        # _loop0_69: codon_type_param
        mark = self._mark()
        children = []
        while codon_type_param_ := self.codon_type_param():
            codon_type_param = Codon.unwrap(codon_type_param_)
            children.append(codon_type_param)
            mark = self._mark()
        self._reset(mark)
        codon_type_param = None
        return children

    def _loop0_70(self) -> List:
        # _loop0_70: codon_type_param
        mark = self._mark()
        children = []
        while codon_type_param_ := self.codon_type_param():
            codon_type_param = Codon.unwrap(codon_type_param_)
            children.append(codon_type_param)
            mark = self._mark()
        self._reset(mark)
        codon_type_param = None
        return children

    def _tmp_71(self) -> Optional:
        # _tmp_71: "int" | "str" | "bool"
        mark = self._mark()
        if literal := self.expect_literal("int"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("str"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("bool"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _loop0_72(self) -> List:
        # _loop0_72: ('not' 'break')
        mark = self._mark()
        children = []
        while _tmp_269_ := self._tmp_269():
            _tmp_269 = Codon.unwrap(_tmp_269_)
            children.append(_tmp_269)
            mark = self._mark()
        self._reset(mark)
        _tmp_269 = None
        return children

    def _loop0_74(self) -> List:
        # _loop0_74: ',' with_item
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.with_item()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_73(self) -> Optional:
        # _gather_73: with_item _loop0_74
        mark = self._mark()
        if (elem := self.with_item()) is not None and (seq := self._loop0_74()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop0_76(self) -> List:
        # _loop0_76: ',' with_item
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.with_item()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_75(self) -> Optional:
        # _gather_75: with_item _loop0_76
        mark = self._mark()
        if (elem := self.with_item()) is not None and (seq := self._loop0_76()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop0_78(self) -> List:
        # _loop0_78: ',' with_item
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.with_item()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_77(self) -> Optional:
        # _gather_77: with_item _loop0_78
        mark = self._mark()
        if (elem := self.with_item()) is not None and (seq := self._loop0_78()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop0_80(self) -> List:
        # _loop0_80: ',' with_item
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.with_item()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_79(self) -> Optional:
        # _gather_79: with_item _loop0_80
        mark = self._mark()
        if (elem := self.with_item()) is not None and (seq := self._loop0_80()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _tmp_81(self) -> Optional:
        # _tmp_81: ',' | ')' | ':'
        mark = self._mark()
        if literal := self.expect_literal(","):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal(")"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal(":"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _loop1_82(self) -> List:
        # _loop1_82: except_block
        mark = self._mark()
        children = []
        while except_block_ := self.except_block():
            except_block = Codon.unwrap(except_block_)
            children.append(except_block)
            mark = self._mark()
        self._reset(mark)
        except_block = None
        return children

    def _tmp_83(self) -> Optional:
        # _tmp_83: 'as' NAME
        mark = self._mark()
        if (self.expect_literal("as")) and (z := self.name()):
            z = Codon.unwrap(z)
            return z.string
        self._reset(mark)
        z = None
        return None

    def _tmp_84(self) -> Optional:
        # _tmp_84: 'as' NAME
        mark = self._mark()
        if (self.expect_literal("as")) and (z := self.name()):
            z = Codon.unwrap(z)
            return z.string
        self._reset(mark)
        z = None
        return None

    def _loop1_85(self) -> List:
        # _loop1_85: case_block
        mark = self._mark()
        children = []
        while case_block_ := self.case_block():
            case_block = Codon.unwrap(case_block_)
            children.append(case_block)
            mark = self._mark()
        self._reset(mark)
        case_block = None
        return children

    def _loop0_87(self) -> List:
        # _loop0_87: '|' closed_pattern
        mark = self._mark()
        children = []
        while (self.expect_literal("|")) and (elem_ := self.closed_pattern()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_86(self) -> Optional:
        # _gather_86: closed_pattern _loop0_87
        mark = self._mark()
        if (elem := self.closed_pattern()) is not None and (seq := self._loop0_87()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _tmp_88(self) -> Optional:
        # _tmp_88: '+' | '-'
        mark = self._mark()
        if literal := self.expect_literal("+"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("-"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_89(self) -> Optional:
        # _tmp_89: '+' | '-'
        mark = self._mark()
        if literal := self.expect_literal("+"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("-"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_90(self) -> Optional:
        # _tmp_90: '.' | '(' | '='
        mark = self._mark()
        if literal := self.expect_literal("."):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("("):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("="):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_91(self) -> Optional:
        # _tmp_91: '.' | '(' | '='
        mark = self._mark()
        if literal := self.expect_literal("."):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("("):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("="):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _loop0_93(self) -> List:
        # _loop0_93: ',' maybe_star_pattern
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.maybe_star_pattern()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_92(self) -> Optional:
        # _gather_92: maybe_star_pattern _loop0_93
        mark = self._mark()
        if (elem := self.maybe_star_pattern()) is not None and (
            seq := self._loop0_93()
        ) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop0_95(self) -> List:
        # _loop0_95: ',' key_value_pattern
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.key_value_pattern()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_94(self) -> Optional:
        # _gather_94: key_value_pattern _loop0_95
        mark = self._mark()
        if (elem := self.key_value_pattern()) is not None and (
            seq := self._loop0_95()
        ) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _tmp_96(self) -> Optional:
        # _tmp_96: literal_expr | attr
        mark = self._mark()
        if literal_expr := self.literal_expr():
            literal_expr = Codon.unwrap(literal_expr)
            return literal_expr
        self._reset(mark)
        literal_expr = None
        if attr := self.attr():
            attr = Codon.unwrap(attr)
            return attr
        self._reset(mark)
        attr = None
        return None

    def _loop0_98(self) -> List:
        # _loop0_98: ',' pattern
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.pattern()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_97(self) -> Optional:
        # _gather_97: pattern _loop0_98
        mark = self._mark()
        if (elem := self.pattern()) is not None and (seq := self._loop0_98()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop0_100(self) -> List:
        # _loop0_100: ',' keyword_pattern
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.keyword_pattern()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_99(self) -> Optional:
        # _gather_99: keyword_pattern _loop0_100
        mark = self._mark()
        if (elem := self.keyword_pattern()) is not None and (seq := self._loop0_100()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop0_102(self) -> List:
        # _loop0_102: ',' type_param
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.type_param()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_101(self) -> Optional:
        # _gather_101: type_param _loop0_102
        mark = self._mark()
        if (elem := self.type_param()) is not None and (seq := self._loop0_102()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop1_103(self) -> List:
        # _loop1_103: (',' expression)
        mark = self._mark()
        children = []
        while _tmp_270_ := self._tmp_270():
            _tmp_270 = Codon.unwrap(_tmp_270_)
            children.append(_tmp_270)
            mark = self._mark()
        self._reset(mark)
        _tmp_270 = None
        return children

    def _loop1_104(self) -> List:
        # _loop1_104: (',' star_expression)
        mark = self._mark()
        children = []
        while _tmp_271_ := self._tmp_271():
            _tmp_271 = Codon.unwrap(_tmp_271_)
            children.append(_tmp_271)
            mark = self._mark()
        self._reset(mark)
        _tmp_271 = None
        return children

    def _loop0_106(self) -> List:
        # _loop0_106: ',' star_named_expression
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.star_named_expression()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_105(self) -> Optional:
        # _gather_105: star_named_expression _loop0_106
        mark = self._mark()
        if (elem := self.star_named_expression()) is not None and (
            seq := self._loop0_106()
        ) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop1_107(self) -> List:
        # _loop1_107: (pipe_operator disjunction)
        mark = self._mark()
        children = []
        while _tmp_272_ := self._tmp_272():
            _tmp_272 = Codon.unwrap(_tmp_272_)
            children.append(_tmp_272)
            mark = self._mark()
        self._reset(mark)
        _tmp_272 = None
        return children

    def _loop1_108(self) -> List:
        # _loop1_108: ('or' conjunction)
        mark = self._mark()
        children = []
        while _tmp_273_ := self._tmp_273():
            _tmp_273 = Codon.unwrap(_tmp_273_)
            children.append(_tmp_273)
            mark = self._mark()
        self._reset(mark)
        _tmp_273 = None
        return children

    def _loop1_109(self) -> List:
        # _loop1_109: ('and' inversion)
        mark = self._mark()
        children = []
        while _tmp_274_ := self._tmp_274():
            _tmp_274 = Codon.unwrap(_tmp_274_)
            children.append(_tmp_274)
            mark = self._mark()
        self._reset(mark)
        _tmp_274 = None
        return children

    def _loop1_110(self) -> List:
        # _loop1_110: compare_op_bitwise_or_pair
        mark = self._mark()
        children = []
        while compare_op_bitwise_or_pair_ := self.compare_op_bitwise_or_pair():
            compare_op_bitwise_or_pair = Codon.unwrap(compare_op_bitwise_or_pair_)
            children.append(compare_op_bitwise_or_pair)
            mark = self._mark()
        self._reset(mark)
        compare_op_bitwise_or_pair = None
        return children

    def _loop0_112(self) -> List:
        # _loop0_112: ',' (slice | starred_expression)
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self._tmp_275()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_111(self) -> Optional:
        # _gather_111: (slice | starred_expression) _loop0_112
        mark = self._mark()
        if (elem := self._tmp_275()) is not None and (seq := self._loop0_112()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _tmp_113(self) -> Optional:
        # _tmp_113: ':' expression?
        mark = self._mark()
        if (self.expect_literal(":")) and (d := self.expression(),):
            return d
        self._reset(mark)
        d = None
        return None

    def _tmp_114(self) -> Optional:
        # _tmp_114: STRING | FSTRING_START | STRING_PREFIX
        mark = self._mark()
        if string := self.string():
            string = Codon.unwrap(string)
            return string
        self._reset(mark)
        string = None
        if fstring_start := self.fstring_start():
            fstring_start = Codon.unwrap(fstring_start)
            return fstring_start
        self._reset(mark)
        fstring_start = None
        if string_prefix := self.string_prefix():
            string_prefix = Codon.unwrap(string_prefix)
            return string_prefix
        self._reset(mark)
        string_prefix = None
        return None

    def _tmp_115(self) -> Optional:
        # _tmp_115: tuple | group | genexp
        mark = self._mark()
        if tuple := self.tuple():
            tuple = Codon.unwrap(tuple)
            return tuple
        self._reset(mark)
        tuple = None
        if group := self.group():
            group = Codon.unwrap(group)
            return group
        self._reset(mark)
        group = None
        if genexp := self.genexp():
            genexp = Codon.unwrap(genexp)
            return genexp
        self._reset(mark)
        genexp = None
        return None

    def _tmp_116(self) -> Optional:
        # _tmp_116: list | listcomp
        mark = self._mark()
        if list := self.list():
            list = Codon.unwrap(list)
            return list
        self._reset(mark)
        list = None
        if listcomp := self.listcomp():
            listcomp = Codon.unwrap(listcomp)
            return listcomp
        self._reset(mark)
        listcomp = None
        return None

    def _tmp_117(self) -> Optional:
        # _tmp_117: dict | set | dictcomp | setcomp
        mark = self._mark()
        if dict := self.dict():
            dict = Codon.unwrap(dict)
            return dict
        self._reset(mark)
        dict = None
        if set := self.set():
            set = Codon.unwrap(set)
            return set
        self._reset(mark)
        set = None
        if dictcomp := self.dictcomp():
            dictcomp = Codon.unwrap(dictcomp)
            return dictcomp
        self._reset(mark)
        dictcomp = None
        if setcomp := self.setcomp():
            setcomp = Codon.unwrap(setcomp)
            return setcomp
        self._reset(mark)
        setcomp = None
        return None

    def _tmp_118(self) -> Optional:
        # _tmp_118: yield_expr | named_expression
        mark = self._mark()
        if yield_expr := self.yield_expr():
            yield_expr = Codon.unwrap(yield_expr)
            return yield_expr
        self._reset(mark)
        yield_expr = None
        if named_expression := self.named_expression():
            named_expression = Codon.unwrap(named_expression)
            return named_expression
        self._reset(mark)
        named_expression = None
        return None

    def _loop0_119(self) -> List:
        # _loop0_119: lambda_param_no_default
        mark = self._mark()
        children = []
        while lambda_param_no_default_ := self.lambda_param_no_default():
            lambda_param_no_default = Codon.unwrap(lambda_param_no_default_)
            children.append(lambda_param_no_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_no_default = None
        return children

    def _loop0_120(self) -> List:
        # _loop0_120: lambda_param_with_default
        mark = self._mark()
        children = []
        while lambda_param_with_default_ := self.lambda_param_with_default():
            lambda_param_with_default = Codon.unwrap(lambda_param_with_default_)
            children.append(lambda_param_with_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_with_default = None
        return children

    def _loop0_121(self) -> List:
        # _loop0_121: lambda_param_with_default
        mark = self._mark()
        children = []
        while lambda_param_with_default_ := self.lambda_param_with_default():
            lambda_param_with_default = Codon.unwrap(lambda_param_with_default_)
            children.append(lambda_param_with_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_with_default = None
        return children

    def _loop1_122(self) -> List:
        # _loop1_122: lambda_param_no_default
        mark = self._mark()
        children = []
        while lambda_param_no_default_ := self.lambda_param_no_default():
            lambda_param_no_default = Codon.unwrap(lambda_param_no_default_)
            children.append(lambda_param_no_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_no_default = None
        return children

    def _loop0_123(self) -> List:
        # _loop0_123: lambda_param_with_default
        mark = self._mark()
        children = []
        while lambda_param_with_default_ := self.lambda_param_with_default():
            lambda_param_with_default = Codon.unwrap(lambda_param_with_default_)
            children.append(lambda_param_with_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_with_default = None
        return children

    def _loop1_124(self) -> List:
        # _loop1_124: lambda_param_with_default
        mark = self._mark()
        children = []
        while lambda_param_with_default_ := self.lambda_param_with_default():
            lambda_param_with_default = Codon.unwrap(lambda_param_with_default_)
            children.append(lambda_param_with_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_with_default = None
        return children

    def _loop1_125(self) -> List:
        # _loop1_125: lambda_param_no_default
        mark = self._mark()
        children = []
        while lambda_param_no_default_ := self.lambda_param_no_default():
            lambda_param_no_default = Codon.unwrap(lambda_param_no_default_)
            children.append(lambda_param_no_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_no_default = None
        return children

    def _loop1_126(self) -> List:
        # _loop1_126: lambda_param_no_default
        mark = self._mark()
        children = []
        while lambda_param_no_default_ := self.lambda_param_no_default():
            lambda_param_no_default = Codon.unwrap(lambda_param_no_default_)
            children.append(lambda_param_no_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_no_default = None
        return children

    def _loop0_127(self) -> List:
        # _loop0_127: lambda_param_no_default
        mark = self._mark()
        children = []
        while lambda_param_no_default_ := self.lambda_param_no_default():
            lambda_param_no_default = Codon.unwrap(lambda_param_no_default_)
            children.append(lambda_param_no_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_no_default = None
        return children

    def _loop1_128(self) -> List:
        # _loop1_128: lambda_param_with_default
        mark = self._mark()
        children = []
        while lambda_param_with_default_ := self.lambda_param_with_default():
            lambda_param_with_default = Codon.unwrap(lambda_param_with_default_)
            children.append(lambda_param_with_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_with_default = None
        return children

    def _loop0_129(self) -> List:
        # _loop0_129: lambda_param_no_default
        mark = self._mark()
        children = []
        while lambda_param_no_default_ := self.lambda_param_no_default():
            lambda_param_no_default = Codon.unwrap(lambda_param_no_default_)
            children.append(lambda_param_no_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_no_default = None
        return children

    def _loop1_130(self) -> List:
        # _loop1_130: lambda_param_with_default
        mark = self._mark()
        children = []
        while lambda_param_with_default_ := self.lambda_param_with_default():
            lambda_param_with_default = Codon.unwrap(lambda_param_with_default_)
            children.append(lambda_param_with_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_with_default = None
        return children

    def _loop0_131(self) -> List:
        # _loop0_131: lambda_param_maybe_default
        mark = self._mark()
        children = []
        while lambda_param_maybe_default_ := self.lambda_param_maybe_default():
            lambda_param_maybe_default = Codon.unwrap(lambda_param_maybe_default_)
            children.append(lambda_param_maybe_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_maybe_default = None
        return children

    def _loop1_132(self) -> List:
        # _loop1_132: lambda_param_maybe_default
        mark = self._mark()
        children = []
        while lambda_param_maybe_default_ := self.lambda_param_maybe_default():
            lambda_param_maybe_default = Codon.unwrap(lambda_param_maybe_default_)
            children.append(lambda_param_maybe_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_maybe_default = None
        return children

    def _tmp_133(self) -> Optional:
        # _tmp_133: yield_expr | star_expressions
        mark = self._mark()
        if yield_expr := self.yield_expr():
            yield_expr = Codon.unwrap(yield_expr)
            return yield_expr
        self._reset(mark)
        yield_expr = None
        if star_expressions := self.star_expressions():
            star_expressions = Codon.unwrap(star_expressions)
            return star_expressions
        self._reset(mark)
        star_expressions = None
        return None

    def _loop0_134(self) -> List:
        # _loop0_134: fstring_format_spec
        mark = self._mark()
        children = []
        while fstring_format_spec_ := self.fstring_format_spec():
            fstring_format_spec = Codon.unwrap(fstring_format_spec_)
            children.append(fstring_format_spec)
            mark = self._mark()
        self._reset(mark)
        fstring_format_spec = None
        return children

    def _tmp_135(self) -> Optional:
        # _tmp_135: '+' | '-'
        mark = self._mark()
        if literal := self.expect_literal("+"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("-"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_136(self) -> Optional:
        # _tmp_136: '<' | '>' | '=' | '^'
        mark = self._mark()
        if literal := self.expect_literal("<"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal(">"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("="):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("^"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_137(self) -> Optional:
        # _tmp_137: NUMBER? fstring_grouping?
        mark = self._mark()
        if (n := self.number(),) and (g := self.fstring_grouping(),):
            return (n.string if n else "") + (g or "")
        self._reset(mark)
        n = None
        g = None
        return None

    def _loop1_138(self) -> List:
        # _loop1_138: (any_string)
        mark = self._mark()
        children = []
        while any_string_ := self.any_string():
            any_string = Codon.unwrap(any_string_)
            children.append(any_string)
            mark = self._mark()
        self._reset(mark)
        any_string = None
        return children

    def _tmp_139(self) -> Optional:
        # _tmp_139: star_named_expression ',' star_named_expressions?
        mark = self._mark()
        if (
            (y := self.star_named_expression())
            and (self.expect_literal(","))
            and (z := self.star_named_expressions(),)
        ):
            y = Codon.unwrap(y)
            return [y] + (z or [])
        self._reset(mark)
        y = None
        z = None
        return None

    def _loop0_141(self) -> List:
        # _loop0_141: ',' double_starred_kvpair
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.double_starred_kvpair()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_140(self) -> Optional:
        # _gather_140: double_starred_kvpair _loop0_141
        mark = self._mark()
        if (elem := self.double_starred_kvpair()) is not None and (
            seq := self._loop0_141()
        ) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop1_142(self) -> List:
        # _loop1_142: for_if_clause
        mark = self._mark()
        children = []
        while for_if_clause_ := self.for_if_clause():
            for_if_clause = Codon.unwrap(for_if_clause_)
            children.append(for_if_clause)
            mark = self._mark()
        self._reset(mark)
        for_if_clause = None
        return children

    def _loop0_143(self) -> List:
        # _loop0_143: ('if' disjunction)
        mark = self._mark()
        children = []
        while _tmp_276_ := self._tmp_276():
            _tmp_276 = Codon.unwrap(_tmp_276_)
            children.append(_tmp_276)
            mark = self._mark()
        self._reset(mark)
        _tmp_276 = None
        return children

    def _loop0_144(self) -> List:
        # _loop0_144: ('if' disjunction)
        mark = self._mark()
        children = []
        while _tmp_277_ := self._tmp_277():
            _tmp_277 = Codon.unwrap(_tmp_277_)
            children.append(_tmp_277)
            mark = self._mark()
        self._reset(mark)
        _tmp_277 = None
        return children

    def _tmp_145(self) -> Optional:
        # _tmp_145: assignment_expression | expression !':='
        mark = self._mark()
        if assignment_expression := self.assignment_expression():
            assignment_expression = Codon.unwrap(assignment_expression)
            return assignment_expression
        self._reset(mark)
        assignment_expression = None
        if (expression := self.expression()) and (
            self.negative_lookahead(self.expect_literal, ":=")
        ):
            expression = Codon.unwrap(expression)
            return expression
        self._reset(mark)
        expression = None
        return None

    def _loop0_147(self) -> List:
        # _loop0_147: ',' (starred_expression | (assignment_expression | expression !':=') !'=')
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self._tmp_278()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_146(self) -> Optional:
        # _gather_146: (starred_expression | (assignment_expression | expression !':=') !'=') _loop0_147
        mark = self._mark()
        if (elem := self._tmp_278()) is not None and (seq := self._loop0_147()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _tmp_148(self) -> Optional:
        # _tmp_148: ',' kwargs
        mark = self._mark()
        if (self.expect_literal(",")) and (k := self.kwargs()):
            k = Codon.unwrap(k)
            return k
        self._reset(mark)
        k = None
        return None

    def _loop0_150(self) -> List:
        # _loop0_150: ',' kwarg_or_starred
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.kwarg_or_starred()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_149(self) -> Optional:
        # _gather_149: kwarg_or_starred _loop0_150
        mark = self._mark()
        if (elem := self.kwarg_or_starred()) is not None and (
            seq := self._loop0_150()
        ) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop0_152(self) -> List:
        # _loop0_152: ',' kwarg_or_double_starred
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.kwarg_or_double_starred()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_151(self) -> Optional:
        # _gather_151: kwarg_or_double_starred _loop0_152
        mark = self._mark()
        if (elem := self.kwarg_or_double_starred()) is not None and (
            seq := self._loop0_152()
        ) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop0_154(self) -> List:
        # _loop0_154: ',' kwarg_or_starred
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.kwarg_or_starred()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_153(self) -> Optional:
        # _gather_153: kwarg_or_starred _loop0_154
        mark = self._mark()
        if (elem := self.kwarg_or_starred()) is not None and (
            seq := self._loop0_154()
        ) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop0_156(self) -> List:
        # _loop0_156: ',' kwarg_or_double_starred
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.kwarg_or_double_starred()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_155(self) -> Optional:
        # _gather_155: kwarg_or_double_starred _loop0_156
        mark = self._mark()
        if (elem := self.kwarg_or_double_starred()) is not None and (
            seq := self._loop0_156()
        ) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop0_157(self) -> List:
        # _loop0_157: (',' star_target)
        mark = self._mark()
        children = []
        while _tmp_279_ := self._tmp_279():
            _tmp_279 = Codon.unwrap(_tmp_279_)
            children.append(_tmp_279)
            mark = self._mark()
        self._reset(mark)
        _tmp_279 = None
        return children

    def _loop0_159(self) -> List:
        # _loop0_159: ',' star_target
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.star_target()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_158(self) -> Optional:
        # _gather_158: star_target _loop0_159
        mark = self._mark()
        if (elem := self.star_target()) is not None and (seq := self._loop0_159()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop1_160(self) -> List:
        # _loop1_160: (',' star_target)
        mark = self._mark()
        children = []
        while _tmp_280_ := self._tmp_280():
            _tmp_280 = Codon.unwrap(_tmp_280_)
            children.append(_tmp_280)
            mark = self._mark()
        self._reset(mark)
        _tmp_280 = None
        return children

    def _tmp_161(self) -> Optional:
        # _tmp_161: !'*' star_target
        mark = self._mark()
        if (self.negative_lookahead(self.expect_literal, "*")) and (
            star_target := self.star_target()
        ):
            star_target = Codon.unwrap(star_target)
            return star_target
        self._reset(mark)
        star_target = None
        return None

    def _loop0_163(self) -> List:
        # _loop0_163: ',' del_target
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.del_target()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_162(self) -> Optional:
        # _gather_162: del_target _loop0_163
        mark = self._mark()
        if (elem := self.del_target()) is not None and (seq := self._loop0_163()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _tmp_164(self) -> Optional:
        # _tmp_164: (','.(starred_expression | (assignment_expression | expression !':=') !'=')+ ',' kwargs) | kwargs
        mark = self._mark()
        if _tmp_281 := self._tmp_281():
            _tmp_281 = Codon.unwrap(_tmp_281)
            return _tmp_281
        self._reset(mark)
        _tmp_281 = None
        if kwargs := self.kwargs():
            kwargs = Codon.unwrap(kwargs)
            return kwargs
        self._reset(mark)
        kwargs = None
        return None

    def _loop0_166(self) -> List:
        # _loop0_166: ',' (starred_expression !'=')
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self._tmp_282()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_165(self) -> Optional:
        # _gather_165: (starred_expression !'=') _loop0_166
        mark = self._mark()
        if (elem := self._tmp_282()) is not None and (seq := self._loop0_166()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _tmp_167(self) -> Optional:
        # _tmp_167: args | expression for_if_clauses
        mark = self._mark()
        if self.args():
            return True
        self._reset(mark)
        if (self.expression()) and (self.for_if_clauses()):
            return True
        self._reset(mark)
        return None

    def _tmp_168(self) -> Optional:
        # _tmp_168: args ','
        mark = self._mark()
        if (ar := self.args()) and (self.expect_literal(",")):
            ar = Codon.unwrap(ar)
            return ar
        self._reset(mark)
        ar = None
        return None

    def _tmp_169(self) -> Optional:
        # _tmp_169: ',' | ')'
        mark = self._mark()
        if literal := self.expect_literal(","):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal(")"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_170(self) -> Optional:
        # _tmp_170: 'True' | 'False' | 'None'
        mark = self._mark()
        if literal := self.expect_literal("True"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("False"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("None"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_171(self) -> Optional:
        # _tmp_171: NAME '='
        mark = self._mark()
        if (name := self.name()) and (literal := self.expect_literal("=")):
            name = Codon.unwrap(name)
            literal = Codon.unwrap(literal)
            return [name, literal]
        self._reset(mark)
        name = None
        literal = None
        return None

    def _tmp_172(self) -> Optional:
        # _tmp_172: NAME STRING | SOFT_KEYWORD
        mark = self._mark()
        if (n := self.name()) and (self.string()):
            n = Codon.unwrap(n)
            return n
        self._reset(mark)
        n = None
        if soft_keyword := self.soft_keyword():
            soft_keyword = Codon.unwrap(soft_keyword)
            return soft_keyword
        self._reset(mark)
        soft_keyword = None
        return None

    def _tmp_173(self) -> Optional:
        # _tmp_173: 'else' | ':'
        mark = self._mark()
        if literal := self.expect_literal("else"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal(":"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_174(self) -> Optional:
        # _tmp_174: FSTRING_MIDDLE | fstring_replacement_field
        mark = self._mark()
        if self.fstring_middle():
            return True
        self._reset(mark)
        if self.fstring_replacement_field():
            return True
        self._reset(mark)
        return None

    def _tmp_175(self) -> Optional:
        # _tmp_175: '=' | ':='
        mark = self._mark()
        if literal := self.expect_literal("="):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal(":="):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_176(self) -> Optional:
        # _tmp_176: (list | tuple | genexp) | ('True' | 'None' | 'False')
        mark = self._mark()
        if self._tmp_283():
            return True
        self._reset(mark)
        if self._tmp_284():
            return True
        self._reset(mark)
        return None

    def _tmp_177(self) -> Optional:
        # _tmp_177: '=' | ':='
        mark = self._mark()
        if literal := self.expect_literal("="):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal(":="):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _loop0_178(self) -> List:
        # _loop0_178: star_named_expressions
        mark = self._mark()
        children = []
        while star_named_expressions_ := self.star_named_expressions():
            star_named_expressions = Codon.unwrap(star_named_expressions_)
            children.append(star_named_expressions)
            mark = self._mark()
        self._reset(mark)
        star_named_expressions = None
        return children

    def _loop0_179(self) -> List:
        # _loop0_179: (star_targets '=')
        mark = self._mark()
        children = []
        while _tmp_285_ := self._tmp_285():
            _tmp_285 = Codon.unwrap(_tmp_285_)
            children.append(_tmp_285)
            mark = self._mark()
        self._reset(mark)
        _tmp_285 = None
        return children

    def _loop0_180(self) -> List:
        # _loop0_180: (star_targets '=')
        mark = self._mark()
        children = []
        while _tmp_286_ := self._tmp_286():
            _tmp_286 = Codon.unwrap(_tmp_286_)
            children.append(_tmp_286)
            mark = self._mark()
        self._reset(mark)
        _tmp_286 = None
        return children

    def _tmp_181(self) -> Optional:
        # _tmp_181: yield_expr | star_expressions
        mark = self._mark()
        if yield_expr := self.yield_expr():
            yield_expr = Codon.unwrap(yield_expr)
            return yield_expr
        self._reset(mark)
        yield_expr = None
        if star_expressions := self.star_expressions():
            star_expressions = Codon.unwrap(star_expressions)
            return star_expressions
        self._reset(mark)
        star_expressions = None
        return None

    def _tmp_182(self) -> Optional:
        # _tmp_182: '[' | '(' | '{'
        mark = self._mark()
        if literal := self.expect_literal("["):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("("):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("{"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_183(self) -> Optional:
        # _tmp_183: '[' | '{'
        mark = self._mark()
        if literal := self.expect_literal("["):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("{"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_184(self) -> Optional:
        # _tmp_184: '[' | '{'
        mark = self._mark()
        if literal := self.expect_literal("["):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("{"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_185(self) -> Optional:
        # _tmp_185: slash_no_default | slash_with_default
        mark = self._mark()
        if slash_no_default := self.slash_no_default():
            slash_no_default = Codon.unwrap(slash_no_default)
            return slash_no_default
        self._reset(mark)
        slash_no_default = None
        if slash_with_default := self.slash_with_default():
            slash_with_default = Codon.unwrap(slash_with_default)
            return slash_with_default
        self._reset(mark)
        slash_with_default = None
        return None

    def _loop0_186(self) -> List:
        # _loop0_186: param_maybe_default
        mark = self._mark()
        children = []
        while param_maybe_default_ := self.param_maybe_default():
            param_maybe_default = Codon.unwrap(param_maybe_default_)
            children.append(param_maybe_default)
            mark = self._mark()
        self._reset(mark)
        param_maybe_default = None
        return children

    def _loop0_187(self) -> List:
        # _loop0_187: param_no_default
        mark = self._mark()
        children = []
        while param_no_default_ := self.param_no_default():
            param_no_default = Codon.unwrap(param_no_default_)
            children.append(param_no_default)
            mark = self._mark()
        self._reset(mark)
        param_no_default = None
        return children

    def _loop0_188(self) -> List:
        # _loop0_188: param_no_default
        mark = self._mark()
        children = []
        while param_no_default_ := self.param_no_default():
            param_no_default = Codon.unwrap(param_no_default_)
            children.append(param_no_default)
            mark = self._mark()
        self._reset(mark)
        param_no_default = None
        return children

    def _loop1_189(self) -> List:
        # _loop1_189: param_no_default
        mark = self._mark()
        children = []
        while param_no_default_ := self.param_no_default():
            param_no_default = Codon.unwrap(param_no_default_)
            children.append(param_no_default)
            mark = self._mark()
        self._reset(mark)
        param_no_default = None
        return children

    def _tmp_190(self) -> Optional:
        # _tmp_190: slash_no_default | slash_with_default
        mark = self._mark()
        if slash_no_default := self.slash_no_default():
            slash_no_default = Codon.unwrap(slash_no_default)
            return slash_no_default
        self._reset(mark)
        slash_no_default = None
        if slash_with_default := self.slash_with_default():
            slash_with_default = Codon.unwrap(slash_with_default)
            return slash_with_default
        self._reset(mark)
        slash_with_default = None
        return None

    def _loop0_191(self) -> List:
        # _loop0_191: param_maybe_default
        mark = self._mark()
        children = []
        while param_maybe_default_ := self.param_maybe_default():
            param_maybe_default = Codon.unwrap(param_maybe_default_)
            children.append(param_maybe_default)
            mark = self._mark()
        self._reset(mark)
        param_maybe_default = None
        return children

    def _tmp_192(self) -> Optional:
        # _tmp_192: ',' | param_no_default
        mark = self._mark()
        if self.expect_literal(","):
            return True
        self._reset(mark)
        if self.param_no_default():
            return True
        self._reset(mark)
        return None

    def _loop0_193(self) -> List:
        # _loop0_193: param_maybe_default
        mark = self._mark()
        children = []
        while param_maybe_default_ := self.param_maybe_default():
            param_maybe_default = Codon.unwrap(param_maybe_default_)
            children.append(param_maybe_default)
            mark = self._mark()
        self._reset(mark)
        param_maybe_default = None
        return children

    def _loop1_194(self) -> List:
        # _loop1_194: param_maybe_default
        mark = self._mark()
        children = []
        while param_maybe_default_ := self.param_maybe_default():
            param_maybe_default = Codon.unwrap(param_maybe_default_)
            children.append(param_maybe_default)
            mark = self._mark()
        self._reset(mark)
        param_maybe_default = None
        return children

    def _tmp_195(self) -> Optional:
        # _tmp_195: ')' | ','
        mark = self._mark()
        if literal := self.expect_literal(")"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal(","):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_196(self) -> Optional:
        # _tmp_196: ')' | (',' (')' | '**'))
        mark = self._mark()
        if literal := self.expect_literal(")"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if _tmp_287 := self._tmp_287():
            _tmp_287 = Codon.unwrap(_tmp_287)
            return _tmp_287
        self._reset(mark)
        _tmp_287 = None
        return None

    def _tmp_197(self) -> Optional:
        # _tmp_197: param_no_default | ','
        mark = self._mark()
        if self.param_no_default():
            return True
        self._reset(mark)
        if self.expect_literal(","):
            return True
        self._reset(mark)
        return None

    def _loop0_198(self) -> List:
        # _loop0_198: param_maybe_default
        mark = self._mark()
        children = []
        while param_maybe_default_ := self.param_maybe_default():
            param_maybe_default = Codon.unwrap(param_maybe_default_)
            children.append(param_maybe_default)
            mark = self._mark()
        self._reset(mark)
        param_maybe_default = None
        return children

    def _tmp_199(self) -> Optional:
        # _tmp_199: param_no_default | ','
        mark = self._mark()
        if self.param_no_default():
            return True
        self._reset(mark)
        if self.expect_literal(","):
            return True
        self._reset(mark)
        return None

    def _tmp_200(self) -> Optional:
        # _tmp_200: '*' | '**' | '/'
        mark = self._mark()
        if literal := self.expect_literal("*"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("**"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("/"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _loop1_201(self) -> List:
        # _loop1_201: param_with_default
        mark = self._mark()
        children = []
        while param_with_default_ := self.param_with_default():
            param_with_default = Codon.unwrap(param_with_default_)
            children.append(param_with_default)
            mark = self._mark()
        self._reset(mark)
        param_with_default = None
        return children

    def _tmp_202(self) -> Optional:
        # _tmp_202: lambda_slash_no_default | lambda_slash_with_default
        mark = self._mark()
        if lambda_slash_no_default := self.lambda_slash_no_default():
            lambda_slash_no_default = Codon.unwrap(lambda_slash_no_default)
            return lambda_slash_no_default
        self._reset(mark)
        lambda_slash_no_default = None
        if lambda_slash_with_default := self.lambda_slash_with_default():
            lambda_slash_with_default = Codon.unwrap(lambda_slash_with_default)
            return lambda_slash_with_default
        self._reset(mark)
        lambda_slash_with_default = None
        return None

    def _loop0_203(self) -> List:
        # _loop0_203: lambda_param_maybe_default
        mark = self._mark()
        children = []
        while lambda_param_maybe_default_ := self.lambda_param_maybe_default():
            lambda_param_maybe_default = Codon.unwrap(lambda_param_maybe_default_)
            children.append(lambda_param_maybe_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_maybe_default = None
        return children

    def _loop0_204(self) -> List:
        # _loop0_204: lambda_param_no_default
        mark = self._mark()
        children = []
        while lambda_param_no_default_ := self.lambda_param_no_default():
            lambda_param_no_default = Codon.unwrap(lambda_param_no_default_)
            children.append(lambda_param_no_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_no_default = None
        return children

    def _loop0_205(self) -> List:
        # _loop0_205: lambda_param_no_default
        mark = self._mark()
        children = []
        while lambda_param_no_default_ := self.lambda_param_no_default():
            lambda_param_no_default = Codon.unwrap(lambda_param_no_default_)
            children.append(lambda_param_no_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_no_default = None
        return children

    def _loop0_207(self) -> List:
        # _loop0_207: ',' lambda_param
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.lambda_param()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_206(self) -> Optional:
        # _gather_206: lambda_param _loop0_207
        mark = self._mark()
        if (elem := self.lambda_param()) is not None and (seq := self._loop0_207()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _tmp_208(self) -> Optional:
        # _tmp_208: lambda_slash_no_default | lambda_slash_with_default
        mark = self._mark()
        if lambda_slash_no_default := self.lambda_slash_no_default():
            lambda_slash_no_default = Codon.unwrap(lambda_slash_no_default)
            return lambda_slash_no_default
        self._reset(mark)
        lambda_slash_no_default = None
        if lambda_slash_with_default := self.lambda_slash_with_default():
            lambda_slash_with_default = Codon.unwrap(lambda_slash_with_default)
            return lambda_slash_with_default
        self._reset(mark)
        lambda_slash_with_default = None
        return None

    def _loop0_209(self) -> List:
        # _loop0_209: lambda_param_maybe_default
        mark = self._mark()
        children = []
        while lambda_param_maybe_default_ := self.lambda_param_maybe_default():
            lambda_param_maybe_default = Codon.unwrap(lambda_param_maybe_default_)
            children.append(lambda_param_maybe_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_maybe_default = None
        return children

    def _tmp_210(self) -> Optional:
        # _tmp_210: ',' | lambda_param_no_default
        mark = self._mark()
        if self.expect_literal(","):
            return True
        self._reset(mark)
        if self.lambda_param_no_default():
            return True
        self._reset(mark)
        return None

    def _loop0_211(self) -> List:
        # _loop0_211: lambda_param_maybe_default
        mark = self._mark()
        children = []
        while lambda_param_maybe_default_ := self.lambda_param_maybe_default():
            lambda_param_maybe_default = Codon.unwrap(lambda_param_maybe_default_)
            children.append(lambda_param_maybe_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_maybe_default = None
        return children

    def _loop1_212(self) -> List:
        # _loop1_212: lambda_param_maybe_default
        mark = self._mark()
        children = []
        while lambda_param_maybe_default_ := self.lambda_param_maybe_default():
            lambda_param_maybe_default = Codon.unwrap(lambda_param_maybe_default_)
            children.append(lambda_param_maybe_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_maybe_default = None
        return children

    def _loop1_213(self) -> List:
        # _loop1_213: lambda_param_with_default
        mark = self._mark()
        children = []
        while lambda_param_with_default_ := self.lambda_param_with_default():
            lambda_param_with_default = Codon.unwrap(lambda_param_with_default_)
            children.append(lambda_param_with_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_with_default = None
        return children

    def _tmp_214(self) -> Optional:
        # _tmp_214: ':' | ',' (':' | '**')
        mark = self._mark()
        if literal := self.expect_literal(":"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if (p := self.expect_literal(",")) and (self._tmp_288()):
            p = Codon.unwrap(p)
            return p
        self._reset(mark)
        p = None
        return None

    def _tmp_215(self) -> Optional:
        # _tmp_215: lambda_param_no_default | ','
        mark = self._mark()
        if self.lambda_param_no_default():
            return True
        self._reset(mark)
        if self.expect_literal(","):
            return True
        self._reset(mark)
        return None

    def _loop0_216(self) -> List:
        # _loop0_216: lambda_param_maybe_default
        mark = self._mark()
        children = []
        while lambda_param_maybe_default_ := self.lambda_param_maybe_default():
            lambda_param_maybe_default = Codon.unwrap(lambda_param_maybe_default_)
            children.append(lambda_param_maybe_default)
            mark = self._mark()
        self._reset(mark)
        lambda_param_maybe_default = None
        return children

    def _tmp_217(self) -> Optional:
        # _tmp_217: lambda_param_no_default | ','
        mark = self._mark()
        if self.lambda_param_no_default():
            return True
        self._reset(mark)
        if self.expect_literal(","):
            return True
        self._reset(mark)
        return None

    def _tmp_218(self) -> Optional:
        # _tmp_218: '*' | '**' | '/'
        mark = self._mark()
        if literal := self.expect_literal("*"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("**"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("/"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_219(self) -> Optional:
        # _tmp_219: ',' | ')' | ':'
        mark = self._mark()
        if literal := self.expect_literal(","):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal(")"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal(":"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _loop0_221(self) -> List:
        # _loop0_221: ',' dotted_name
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.dotted_name()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_220(self) -> Optional:
        # _gather_220: dotted_name _loop0_221
        mark = self._mark()
        if (elem := self.dotted_name()) is not None and (seq := self._loop0_221()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop0_223(self) -> List:
        # _loop0_223: ',' (expression ['as' star_target])
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self._tmp_289()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_222(self) -> Optional:
        # _gather_222: (expression ['as' star_target]) _loop0_223
        mark = self._mark()
        if (elem := self._tmp_289()) is not None and (seq := self._loop0_223()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop0_225(self) -> List:
        # _loop0_225: ',' (expressions ['as' star_target])
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self._tmp_290()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_224(self) -> Optional:
        # _gather_224: (expressions ['as' star_target]) _loop0_225
        mark = self._mark()
        if (elem := self._tmp_290()) is not None and (seq := self._loop0_225()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop0_227(self) -> List:
        # _loop0_227: ',' (expression ['as' star_target])
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self._tmp_291()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_226(self) -> Optional:
        # _gather_226: (expression ['as' star_target]) _loop0_227
        mark = self._mark()
        if (elem := self._tmp_291()) is not None and (seq := self._loop0_227()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _loop0_229(self) -> List:
        # _loop0_229: ',' (expressions ['as' star_target])
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self._tmp_292()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_228(self) -> Optional:
        # _gather_228: (expressions ['as' star_target]) _loop0_229
        mark = self._mark()
        if (elem := self._tmp_292()) is not None and (seq := self._loop0_229()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _tmp_230(self) -> Optional:
        # _tmp_230: 'except' | 'finally'
        mark = self._mark()
        if literal := self.expect_literal("except"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("finally"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _loop0_231(self) -> List:
        # _loop0_231: block
        mark = self._mark()
        children = []
        while block_ := self.block():
            block = Codon.unwrap(block_)
            children.append(block)
            mark = self._mark()
        self._reset(mark)
        block = None
        return children

    def _loop1_232(self) -> List:
        # _loop1_232: except_block
        mark = self._mark()
        children = []
        while except_block_ := self.except_block():
            except_block = Codon.unwrap(except_block_)
            children.append(except_block)
            mark = self._mark()
        self._reset(mark)
        except_block = None
        return children

    def _tmp_233(self) -> Optional:
        # _tmp_233: 'as' NAME
        mark = self._mark()
        if (self.expect_literal("as")) and (self.name()):
            return True
        self._reset(mark)
        return None

    def _loop1_234(self) -> List:
        # _loop1_234: except_star_block
        mark = self._mark()
        children = []
        while except_star_block_ := self.except_star_block():
            except_star_block = Codon.unwrap(except_star_block_)
            children.append(except_star_block)
            mark = self._mark()
        self._reset(mark)
        except_star_block = None
        return children

    def _loop0_235(self) -> List:
        # _loop0_235: block
        mark = self._mark()
        children = []
        while block_ := self.block():
            block = Codon.unwrap(block_)
            children.append(block)
            mark = self._mark()
        self._reset(mark)
        block = None
        return children

    def _loop1_236(self) -> List:
        # _loop1_236: except_star_block
        mark = self._mark()
        children = []
        while except_star_block_ := self.except_star_block():
            except_star_block = Codon.unwrap(except_star_block_)
            children.append(except_star_block)
            mark = self._mark()
        self._reset(mark)
        except_star_block = None
        return children

    def _tmp_237(self) -> Optional:
        # _tmp_237: expression ['as' NAME]
        mark = self._mark()
        if (e := self.expression()) and (self._tmp_293(),):
            e = Codon.unwrap(e)
            return e
        self._reset(mark)
        e = None
        return None

    def _tmp_238(self) -> Optional:
        # _tmp_238: 'as' NAME
        mark = self._mark()
        if (self.expect_literal("as")) and (self.name()):
            return True
        self._reset(mark)
        return None

    def _tmp_239(self) -> Optional:
        # _tmp_239: 'as' NAME
        mark = self._mark()
        if (self.expect_literal("as")) and (self.name()):
            return True
        self._reset(mark)
        return None

    def _tmp_240(self) -> Optional:
        # _tmp_240: NEWLINE | ':'
        mark = self._mark()
        if _newline := self.expect_type(tokenize.Tokens.NEWLINE):
            _newline = Codon.unwrap(_newline)
            return _newline
        self._reset(mark)
        _newline = None
        if literal := self.expect_literal(":"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_241(self) -> Optional:
        # _tmp_241: 'as' NAME
        mark = self._mark()
        if (self.expect_literal("as")) and (self.name()):
            return True
        self._reset(mark)
        return None

    def _tmp_242(self) -> Optional:
        # _tmp_242: 'as' NAME
        mark = self._mark()
        if (self.expect_literal("as")) and (self.name()):
            return True
        self._reset(mark)
        return None

    def _tmp_243(self) -> Optional:
        # _tmp_243: positional_patterns ','
        mark = self._mark()
        if (self.positional_patterns()) and (self.expect_literal(",")):
            return True
        self._reset(mark)
        return None

    def _tmp_244(self) -> Optional:
        # _tmp_244: '->' expression
        mark = self._mark()
        if (self.expect_literal("->")) and (self.expression()):
            return True
        self._reset(mark)
        return None

    def _tmp_245(self) -> Optional:
        # _tmp_245: '(' arguments? ')'
        mark = self._mark()
        if (self.expect_literal("(")) and (self.arguments(),) and (self.expect_literal(")")):
            return True
        self._reset(mark)
        return None

    def _tmp_246(self) -> Optional:
        # _tmp_246: '(' arguments? ')'
        mark = self._mark()
        if (self.expect_literal("(")) and (self.arguments(),) and (self.expect_literal(")")):
            return True
        self._reset(mark)
        return None

    def _loop0_248(self) -> List:
        # _loop0_248: ',' double_starred_kvpair
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self.double_starred_kvpair()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_247(self) -> Optional:
        # _gather_247: double_starred_kvpair _loop0_248
        mark = self._mark()
        if (elem := self.double_starred_kvpair()) is not None and (
            seq := self._loop0_248()
        ) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _tmp_249(self) -> Optional:
        # _tmp_249: '}' | ','
        mark = self._mark()
        if literal := self.expect_literal("}"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal(","):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_250(self) -> Optional:
        # _tmp_250: '}' | ','
        mark = self._mark()
        if literal := self.expect_literal("}"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal(","):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_251(self) -> Optional:
        # _tmp_251: yield_expr | star_expressions
        mark = self._mark()
        if yield_expr := self.yield_expr():
            yield_expr = Codon.unwrap(yield_expr)
            return yield_expr
        self._reset(mark)
        yield_expr = None
        if star_expressions := self.star_expressions():
            star_expressions = Codon.unwrap(star_expressions)
            return star_expressions
        self._reset(mark)
        star_expressions = None
        return None

    def _tmp_252(self) -> Optional:
        # _tmp_252: yield_expr | star_expressions
        mark = self._mark()
        if yield_expr := self.yield_expr():
            yield_expr = Codon.unwrap(yield_expr)
            return yield_expr
        self._reset(mark)
        yield_expr = None
        if star_expressions := self.star_expressions():
            star_expressions = Codon.unwrap(star_expressions)
            return star_expressions
        self._reset(mark)
        star_expressions = None
        return None

    def _tmp_253(self) -> Optional:
        # _tmp_253: '=' | '!' | ':' | '}'
        mark = self._mark()
        if literal := self.expect_literal("="):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("!"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal(":"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("}"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_254(self) -> Optional:
        # _tmp_254: yield_expr | star_expressions
        mark = self._mark()
        if yield_expr := self.yield_expr():
            yield_expr = Codon.unwrap(yield_expr)
            return yield_expr
        self._reset(mark)
        yield_expr = None
        if star_expressions := self.star_expressions():
            star_expressions = Codon.unwrap(star_expressions)
            return star_expressions
        self._reset(mark)
        star_expressions = None
        return None

    def _tmp_255(self) -> Optional:
        # _tmp_255: '!' | ':' | '}'
        mark = self._mark()
        if literal := self.expect_literal("!"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal(":"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("}"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_256(self) -> Optional:
        # _tmp_256: yield_expr | star_expressions
        mark = self._mark()
        if yield_expr := self.yield_expr():
            yield_expr = Codon.unwrap(yield_expr)
            return yield_expr
        self._reset(mark)
        yield_expr = None
        if star_expressions := self.star_expressions():
            star_expressions = Codon.unwrap(star_expressions)
            return star_expressions
        self._reset(mark)
        star_expressions = None
        return None

    def _tmp_257(self) -> Optional:
        # _tmp_257: yield_expr | star_expressions
        mark = self._mark()
        if yield_expr := self.yield_expr():
            yield_expr = Codon.unwrap(yield_expr)
            return yield_expr
        self._reset(mark)
        yield_expr = None
        if star_expressions := self.star_expressions():
            star_expressions = Codon.unwrap(star_expressions)
            return star_expressions
        self._reset(mark)
        star_expressions = None
        return None

    def _tmp_258(self) -> Optional:
        # _tmp_258: '!' NAME
        mark = self._mark()
        if (literal := self.expect_literal("!")) and (name := self.name()):
            literal = Codon.unwrap(literal)
            name = Codon.unwrap(name)
            return [literal, name]
        self._reset(mark)
        literal = None
        name = None
        return None

    def _tmp_259(self) -> Optional:
        # _tmp_259: ':' | '}'
        mark = self._mark()
        if literal := self.expect_literal(":"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("}"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_260(self) -> Optional:
        # _tmp_260: yield_expr | star_expressions
        mark = self._mark()
        if yield_expr := self.yield_expr():
            yield_expr = Codon.unwrap(yield_expr)
            return yield_expr
        self._reset(mark)
        yield_expr = None
        if star_expressions := self.star_expressions():
            star_expressions = Codon.unwrap(star_expressions)
            return star_expressions
        self._reset(mark)
        star_expressions = None
        return None

    def _tmp_261(self) -> Optional:
        # _tmp_261: '!' NAME
        mark = self._mark()
        if (literal := self.expect_literal("!")) and (name := self.name()):
            literal = Codon.unwrap(literal)
            name = Codon.unwrap(name)
            return [literal, name]
        self._reset(mark)
        literal = None
        name = None
        return None

    def _loop0_262(self) -> List:
        # _loop0_262: fstring_format_spec
        mark = self._mark()
        children = []
        while fstring_format_spec_ := self.fstring_format_spec():
            fstring_format_spec = Codon.unwrap(fstring_format_spec_)
            children.append(fstring_format_spec)
            mark = self._mark()
        self._reset(mark)
        fstring_format_spec = None
        return children

    def _tmp_263(self) -> Optional:
        # _tmp_263: yield_expr | star_expressions
        mark = self._mark()
        if yield_expr := self.yield_expr():
            yield_expr = Codon.unwrap(yield_expr)
            return yield_expr
        self._reset(mark)
        yield_expr = None
        if star_expressions := self.star_expressions():
            star_expressions = Codon.unwrap(star_expressions)
            return star_expressions
        self._reset(mark)
        star_expressions = None
        return None

    def _tmp_264(self) -> Optional:
        # _tmp_264: '!' NAME
        mark = self._mark()
        if (literal := self.expect_literal("!")) and (name := self.name()):
            literal = Codon.unwrap(literal)
            name = Codon.unwrap(name)
            return [literal, name]
        self._reset(mark)
        literal = None
        name = None
        return None

    def _tmp_265(self) -> Optional:
        # _tmp_265: ':' | '}'
        mark = self._mark()
        if literal := self.expect_literal(":"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("}"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_266(self) -> Optional:
        # _tmp_266: star_targets '='
        mark = self._mark()
        if (z := self.star_targets()) and (self.expect_literal("=")):
            z = Codon.unwrap(z)
            return z
        self._reset(mark)
        z = None
        return None

    def _tmp_267(self) -> Optional:
        # _tmp_267: '.' | '...'
        mark = self._mark()
        if literal := self.expect_literal("."):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("..."):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_268(self) -> Optional:
        # _tmp_268: '.' | '...'
        mark = self._mark()
        if literal := self.expect_literal("."):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("..."):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_269(self) -> Optional:
        # _tmp_269: 'not' 'break'
        mark = self._mark()
        if (literal := self.expect_literal("not")) and (literal_1 := self.expect_literal("break")):
            literal = Codon.unwrap(literal)
            literal_1 = Codon.unwrap(literal_1)
            return [literal, literal_1]
        self._reset(mark)
        literal = None
        literal_1 = None
        return None

    def _tmp_270(self) -> Optional:
        # _tmp_270: ',' expression
        mark = self._mark()
        if (self.expect_literal(",")) and (c := self.expression()):
            c = Codon.unwrap(c)
            return c
        self._reset(mark)
        c = None
        return None

    def _tmp_271(self) -> Optional:
        # _tmp_271: ',' star_expression
        mark = self._mark()
        if (self.expect_literal(",")) and (c := self.star_expression()):
            c = Codon.unwrap(c)
            return c
        self._reset(mark)
        c = None
        return None

    def _tmp_272(self) -> Optional:
        # _tmp_272: pipe_operator disjunction
        mark = self._mark()
        if (p := self.pipe_operator()) and (d := self.disjunction()):
            p = Codon.unwrap(p)
            d = Codon.unwrap(d)
            return (p, d)
        self._reset(mark)
        p = None
        d = None
        return None

    def _tmp_273(self) -> Optional:
        # _tmp_273: 'or' conjunction
        mark = self._mark()
        if (self.expect_literal("or")) and (c := self.conjunction()):
            c = Codon.unwrap(c)
            return c
        self._reset(mark)
        c = None
        return None

    def _tmp_274(self) -> Optional:
        # _tmp_274: 'and' inversion
        mark = self._mark()
        if (self.expect_literal("and")) and (c := self.inversion()):
            c = Codon.unwrap(c)
            return c
        self._reset(mark)
        c = None
        return None

    def _tmp_275(self) -> Optional:
        # _tmp_275: slice | starred_expression
        mark = self._mark()
        if slice := self.slice():
            slice = Codon.unwrap(slice)
            return slice
        self._reset(mark)
        slice = None
        if starred_expression := self.starred_expression():
            starred_expression = Codon.unwrap(starred_expression)
            return starred_expression
        self._reset(mark)
        starred_expression = None
        return None

    def _tmp_276(self) -> Optional:
        # _tmp_276: 'if' disjunction
        mark = self._mark()
        if (self.expect_literal("if")) and (z := self.disjunction()):
            z = Codon.unwrap(z)
            return z
        self._reset(mark)
        z = None
        return None

    def _tmp_277(self) -> Optional:
        # _tmp_277: 'if' disjunction
        mark = self._mark()
        if (self.expect_literal("if")) and (z := self.disjunction()):
            z = Codon.unwrap(z)
            return z
        self._reset(mark)
        z = None
        return None

    def _tmp_278(self) -> Optional:
        # _tmp_278: starred_expression | (assignment_expression | expression !':=') !'='
        mark = self._mark()
        if starred_expression := self.starred_expression():
            starred_expression = Codon.unwrap(starred_expression)
            return starred_expression
        self._reset(mark)
        starred_expression = None
        if (_tmp_294 := self._tmp_294()) and (self.negative_lookahead(self.expect_literal, "=")):
            _tmp_294 = Codon.unwrap(_tmp_294)
            return _tmp_294
        self._reset(mark)
        _tmp_294 = None
        return None

    def _tmp_279(self) -> Optional:
        # _tmp_279: ',' star_target
        mark = self._mark()
        if (self.expect_literal(",")) and (c := self.star_target()):
            c = Codon.unwrap(c)
            return c
        self._reset(mark)
        c = None
        return None

    def _tmp_280(self) -> Optional:
        # _tmp_280: ',' star_target
        mark = self._mark()
        if (self.expect_literal(",")) and (c := self.star_target()):
            c = Codon.unwrap(c)
            return c
        self._reset(mark)
        c = None
        return None

    def _tmp_281(self) -> Optional:
        # _tmp_281: ','.(starred_expression | (assignment_expression | expression !':=') !'=')+ ',' kwargs
        mark = self._mark()
        if (self._gather_295()) and (self.expect_literal(",")) and (self.kwargs()):
            return True
        self._reset(mark)
        return None

    def _tmp_282(self) -> Optional:
        # _tmp_282: starred_expression !'='
        mark = self._mark()
        if (starred_expression := self.starred_expression()) and (
            self.negative_lookahead(self.expect_literal, "=")
        ):
            starred_expression = Codon.unwrap(starred_expression)
            return starred_expression
        self._reset(mark)
        starred_expression = None
        return None

    def _tmp_283(self) -> Optional:
        # _tmp_283: list | tuple | genexp
        mark = self._mark()
        if list := self.list():
            list = Codon.unwrap(list)
            return list
        self._reset(mark)
        list = None
        if tuple := self.tuple():
            tuple = Codon.unwrap(tuple)
            return tuple
        self._reset(mark)
        tuple = None
        if genexp := self.genexp():
            genexp = Codon.unwrap(genexp)
            return genexp
        self._reset(mark)
        genexp = None
        return None

    def _tmp_284(self) -> Optional:
        # _tmp_284: 'True' | 'None' | 'False'
        mark = self._mark()
        if literal := self.expect_literal("True"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("None"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("False"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_285(self) -> Optional:
        # _tmp_285: star_targets '='
        mark = self._mark()
        if (self.star_targets()) and (self.expect_literal("=")):
            return True
        self._reset(mark)
        return None

    def _tmp_286(self) -> Optional:
        # _tmp_286: star_targets '='
        mark = self._mark()
        if (self.star_targets()) and (self.expect_literal("=")):
            return True
        self._reset(mark)
        return None

    def _tmp_287(self) -> Optional:
        # _tmp_287: ',' (')' | '**')
        mark = self._mark()
        if (c := self.expect_literal(",")) and (self._tmp_297()):
            c = Codon.unwrap(c)
            return c
        self._reset(mark)
        c = None
        return None

    def _tmp_288(self) -> Optional:
        # _tmp_288: ':' | '**'
        mark = self._mark()
        if literal := self.expect_literal(":"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("**"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_289(self) -> Optional:
        # _tmp_289: expression ['as' star_target]
        mark = self._mark()
        if (expression := self.expression()) and (opt := self._tmp_298(),):
            expression = Codon.unwrap(expression)
            return [expression, opt]
        self._reset(mark)
        expression = None
        opt = None
        return None

    def _tmp_290(self) -> Optional:
        # _tmp_290: expressions ['as' star_target]
        mark = self._mark()
        if (expressions := self.expressions()) and (opt := self._tmp_299(),):
            expressions = Codon.unwrap(expressions)
            return [expressions, opt]
        self._reset(mark)
        expressions = None
        opt = None
        return None

    def _tmp_291(self) -> Optional:
        # _tmp_291: expression ['as' star_target]
        mark = self._mark()
        if (expression := self.expression()) and (opt := self._tmp_300(),):
            expression = Codon.unwrap(expression)
            return [expression, opt]
        self._reset(mark)
        expression = None
        opt = None
        return None

    def _tmp_292(self) -> Optional:
        # _tmp_292: expressions ['as' star_target]
        mark = self._mark()
        if (expressions := self.expressions()) and (opt := self._tmp_301(),):
            expressions = Codon.unwrap(expressions)
            return [expressions, opt]
        self._reset(mark)
        expressions = None
        opt = None
        return None

    def _tmp_293(self) -> Optional:
        # _tmp_293: 'as' NAME
        mark = self._mark()
        if (self.expect_literal("as")) and (self.name()):
            return True
        self._reset(mark)
        return None

    def _tmp_294(self) -> Optional:
        # _tmp_294: assignment_expression | expression !':='
        mark = self._mark()
        if assignment_expression := self.assignment_expression():
            assignment_expression = Codon.unwrap(assignment_expression)
            return assignment_expression
        self._reset(mark)
        assignment_expression = None
        if (expression := self.expression()) and (
            self.negative_lookahead(self.expect_literal, ":=")
        ):
            expression = Codon.unwrap(expression)
            return expression
        self._reset(mark)
        expression = None
        return None

    def _loop0_296(self) -> List:
        # _loop0_296: ',' (starred_expression | (assignment_expression | expression !':=') !'=')
        mark = self._mark()
        children = []
        while (self.expect_literal(",")) and (elem_ := self._tmp_302()):
            elem = Codon.unwrap(elem_)
            children.append(elem)
            mark = self._mark()
        self._reset(mark)
        elem = None
        return children

    def _gather_295(self) -> Optional:
        # _gather_295: (starred_expression | (assignment_expression | expression !':=') !'=') _loop0_296
        mark = self._mark()
        if (elem := self._tmp_302()) is not None and (seq := self._loop0_296()) is not None:
            elem = Codon.unwrap(elem)
            seq = Codon.unwrap(seq)
            return [elem] + seq
        self._reset(mark)
        elem = None
        seq = None
        return None

    def _tmp_297(self) -> Optional:
        # _tmp_297: ')' | '**'
        mark = self._mark()
        if literal := self.expect_literal(")"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        if literal := self.expect_literal("**"):
            literal = Codon.unwrap(literal)
            return literal
        self._reset(mark)
        literal = None
        return None

    def _tmp_298(self) -> Optional:
        # _tmp_298: 'as' star_target
        mark = self._mark()
        if (self.expect_literal("as")) and (s := self.star_target()):
            s = Codon.unwrap(s)
            return s
        self._reset(mark)
        s = None
        return None

    def _tmp_299(self) -> Optional:
        # _tmp_299: 'as' star_target
        mark = self._mark()
        if (self.expect_literal("as")) and (s := self.star_target()):
            s = Codon.unwrap(s)
            return s
        self._reset(mark)
        s = None
        return None

    def _tmp_300(self) -> Optional:
        # _tmp_300: 'as' star_target
        mark = self._mark()
        if (self.expect_literal("as")) and (s := self.star_target()):
            s = Codon.unwrap(s)
            return s
        self._reset(mark)
        s = None
        return None

    def _tmp_301(self) -> Optional:
        # _tmp_301: 'as' star_target
        mark = self._mark()
        if (self.expect_literal("as")) and (s := self.star_target()):
            s = Codon.unwrap(s)
            return s
        self._reset(mark)
        s = None
        return None

    def _tmp_302(self) -> Optional:
        # _tmp_302: starred_expression | (assignment_expression | expression !':=') !'='
        mark = self._mark()
        if starred_expression := self.starred_expression():
            starred_expression = Codon.unwrap(starred_expression)
            return starred_expression
        self._reset(mark)
        starred_expression = None
        if (_tmp_303 := self._tmp_303()) and (self.negative_lookahead(self.expect_literal, "=")):
            _tmp_303 = Codon.unwrap(_tmp_303)
            return _tmp_303
        self._reset(mark)
        _tmp_303 = None
        return None

    def _tmp_303(self) -> Optional:
        # _tmp_303: assignment_expression | expression !':='
        mark = self._mark()
        if assignment_expression := self.assignment_expression():
            assignment_expression = Codon.unwrap(assignment_expression)
            return assignment_expression
        self._reset(mark)
        assignment_expression = None
        if (expression := self.expression()) and (
            self.negative_lookahead(self.expect_literal, ":=")
        ):
            expression = Codon.unwrap(expression)
            return expression
        self._reset(mark)
        expression = None
        return None

    def __init__(self, tokenizer: Tokenizer, verbose: bool = False, filename: str = "<unknown>"):
        super().__init__(tokenizer, verbose, filename)
        self.KEYWORDS = [
            "False",
            "None",
            "True",
            "and",
            "as",
            "assert",
            "async",
            "await",
            "break",
            "class",
            "continue",
            "def",
            "del",
            "elif",
            "else",
            "except",
            "finally",
            "for",
            "from",
            "global",
            "if",
            "import",
            "in",
            "is",
            "lambda",
            "nonlocal",
            "not",
            "or",
            "pass",
            "raise",
            "return",
            "try",
            "while",
            "with",
            "yield",
        ]
        self.SOFT_KEYWORDS = ["Literal", "case", "match", "print"]
