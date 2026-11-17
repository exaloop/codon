# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""Dump determinism and semantic distinctions; no additional Codon programs."""

import unittest

from .. import ast, ir
from .util.format import format_ir


def literal_stores(order, values=(1, 2)):
    module = ir.Module(None)
    typ = module.get_int_type()
    variables = {name: module.nr(ir.Var, typ, True, False, False, name) for name in order}
    body = module.nr(ir.SeriesFlow)
    for name, value in zip(order, values):
        body.push_back(
            module.nr(ir.AssignInstr, variables[name], module.nr(ir.IntConst, value, typ))
        )
    module.main_func.body = body
    return module


def captured_call(order, generator=True):
    module = ir.Module(None)
    typ = module.get_int_type()
    fn = module.nr(ir.BodiedFunc, "closure")
    fn.generator = generator
    fn.realize(module.unsafe_get_func_type("closure", typ, [typ, typ]), order)
    fn.body = module.nr(ir.SeriesFlow)
    fn.body.push_back(module.nr(ir.ReturnInstr, module.nr(ir.VarValue, fn.get_arg_var("a"))))
    args = [module.nr(ir.IntConst, {"a": 1, "b": 2}[name], typ) for name in order]
    module.main_func.body = module.nr(ir.SeriesFlow)
    module.main_func.body.push_back(module.nr(ir.CallInstr, module.nr(ir.VarValue, fn), args))
    return module


class FormatTests(unittest.TestCase):
    def test_ast_dump_layout_and_options(self):
        module = ir.Module(None)
        var = module.nr(ir.Var, module.get_int_type(), True, False, False, "x")
        var.set_attribute(ir.StringValueAttribute("demo"), ast.Attr.Module)
        var.set_attribute(
            ir.KeyValueAttribute({"z": "two", "a": "one"}), ast.Attr.FunctionAttributes
        )
        var.set_attribute(ir.Attribute(), ast.Attr.Test)
        self.assertEqual(var.get_attribute(ast.Attr.Module).value, "demo")
        self.assertTrue(var.has_attribute(ast.Attr.Test))
        # Give ast.dump the equivalent record, so its exact layout is the oracle.
        reference = type("TypeRef", (ast.Node.Attribute,), {})()
        reference.name = "Int64"
        expected = type("Var", (ast.Node.Attribute,), {})()
        expected.name, expected.id, expected.type = "x", var.id, reference
        expected.global_, expected.external, expected.tls = True, False, False
        expected.attributes = {
            ast.Attr.Module: "demo",
            ast.Attr.FunctionAttributes: {"z": "two", "a": "one"},
            ast.Attr.Test: True,
        }
        for indent in (None, 0, 1, 2, 4):
            for names in (True, False):
                for attrs in (True, False):
                    with self.subTest(indent=indent, names=names, attrs=attrs):
                        self.assertEqual(
                            ir.dump(
                                var, annotate_fields=names, include_attributes=attrs, indent=indent
                            ),
                            ast.dump(
                                expected,
                                annotate_fields=names,
                                include_attributes=attrs,
                                indent=indent,
                            ),
                        )

    def test_allocation_and_declaration_order(self):
        first = literal_stores(["a", "b"], (1, 2))
        # Unregistered nodes advance the compiler counter but cannot affect dumps.
        ir.SeriesFlow()
        ir.SeriesFlow()
        second = literal_stores(["b", "a"], (2, 1))
        self.assertEqual(format_ir(first), format_ir(second))

    def test_literal_value_difference_is_visible(self):
        self.assertNotEqual(
            format_ir(literal_stores(["a", "b"], (1, 2))),
            format_ir(literal_stores(["a", "b"], (1, 3))),
        )

    def test_repeated_writes_remain_ordered(self):
        first = literal_stores(["a", "a"], (1, 2))
        second = literal_stores(["a", "a"], (2, 1))
        self.assertNotEqual(format_ir(first), format_ir(second))

    def test_captured_bindings(self):
        self.assertEqual(
            format_ir(captured_call(["a", "b"])),
            format_ir(captured_call(["b", "a"])),
        )

    def test_ordinary_positional_arguments_remain_ordered(self):
        self.assertNotEqual(
            format_ir(captured_call(["a", "b"], False)),
            format_ir(captured_call(["b", "a"], False)),
        )

    def test_dump_does_not_mutate_ir(self):
        module = captured_call(["b", "a"])
        fn = next(var for var in module.vars if isinstance(var, ir.BodiedFunc))
        variables, arguments = list(module.vars), list(fn.args)
        instructions = list(module.main_func.body.series)
        counter = ir.IdMixin.current_id
        format_ir(module)
        self.assertEqual(module.vars, variables)
        self.assertEqual(fn.args, arguments)
        self.assertEqual(module.main_func.body.series, instructions)
        self.assertEqual(ir.IdMixin.current_id, counter)

    def test_replacement_graph_numbering(self):
        def make_module(replace):
            module = ir.Module(None)
            typ = module.get_int_type()
            fn = module.nr(ir.InternalFunc, "unused")
            previous = module.nr(ir.CallInstr, module.nr(ir.VarValue, fn), [])
            current = module.nr(
                ir.FlowInstr, module.nr(ir.SeriesFlow), module.nr(ir.IntConst, 7, typ)
            )
            if replace:
                previous.replace_all(current)
            module.main_func.body = module.nr(ir.SeriesFlow)
            module.main_func.body.push_back(previous if replace else current)
            return module

        self.assertEqual(ir.dump(make_module(True)), ir.dump(make_module(False)))

    def test_instruction_flags_and_partial_slots(self):
        first = literal_stores(["a"], (1,))
        second = literal_stores(["a"], (1,))
        left = first.main_func.body.series[0].rhs
        right = second.main_func.body.series[0].rhs
        first.main_func.set_attribute(ir.PartialFunctionAttribute("f", [left, None]))
        second.main_func.set_attribute(ir.PartialFunctionAttribute("f", [None, right]))
        self.assertNotEqual(
            ir.dump(first, include_attributes=True), ir.dump(second, include_attributes=True)
        )
        final = first.nr(ir.YieldInstr, left, True)
        self.assertIn("final=True", ir.dump(final))
        self.assertIn("suspend=False", ir.dump(first.nr(ir.YieldInInstr, left.type, False)))


if __name__ == "__main__":
    unittest.main()
