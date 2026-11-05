
import json
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MINIC = ROOT / "build" / "minic"


class JsonExportTests(unittest.TestCase):

    def export_ast(self, source):
        with tempfile.TemporaryDirectory() as directory:
            source_file = Path(directory) / "input.c"
            output_file = Path(directory) / "ast.json"

            source_file.write_text(
                textwrap.dedent(source).strip() + "\n",
                encoding="utf-8"
            )

            result = subprocess.run(
                [str(MINIC), "--emit-json",
                 str(source_file), str(output_file)],
                capture_output=True,
                text=True,
                timeout=10
            )

            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(output_file.exists())

            return json.loads(output_file.read_text(encoding="utf-8"))

    def all_nodes(self, node):
        yield node

        for child in node["children"]:
            yield from self.all_nodes(child)

    def test_demo_structure(self):
        source = (ROOT / "examples" / "demo.c").read_text(
            encoding="utf-8"
        )

        tree = self.export_ast(source)

        self.assertEqual(tree["type"], "program")
        self.assertEqual(len(tree["children"]), 1)
        self.assertEqual(tree["children"][0]["type"], "block")
        self.assertEqual(len(list(self.all_nodes(tree))), 33)

    def test_operator_precedence(self):
        tree = self.export_ast("""
            int main() {
                int a = 2 + 3 * 4;
                int b = (2 + 3) * 4;
                return 0;
            }
        """)

        statements = tree["children"][0]["children"]
        first_expression = statements[0]["children"][0]
        second_expression = statements[1]["children"][0]

        self.assertEqual(first_expression["label"], "+")
        self.assertEqual(
            first_expression["children"][1]["label"], "*"
        )

        self.assertEqual(second_expression["label"], "*")
        self.assertEqual(
            second_expression["children"][0]["label"], "+"
        )

    def test_control_flow(self):
        source = (ROOT / "examples" / "demo.c").read_text(
            encoding="utf-8"
        )

        tree = self.export_ast(source)
        nodes = list(self.all_nodes(tree))
        types = [node["type"] for node in nodes]

        self.assertIn("while", types)
        self.assertIn("if", types)
        self.assertIn("assignment", types)
        self.assertIn("return", types)

        if_node = next(
            node for node in nodes if node["type"] == "if"
        )

        self.assertEqual(len(if_node["children"]), 3)

    def test_empty_nested_block(self):
        tree = self.export_ast("""
            int main() {
                {}
                return 0;
            }
        """)

        statements = tree["children"][0]["children"]

        self.assertEqual(statements[0]["type"], "block")
        self.assertEqual(statements[0]["children"], [])
        self.assertEqual(statements[1]["type"], "return")

    def test_printf_label(self):
        tree = self.export_ast(r"""
            int main() {
                printf("%d\n", 42);
                return 0;
            }
        """)

        statements = tree["children"][0]["children"]
        print_node = statements[0]

        self.assertEqual(print_node["type"], "printf")
        self.assertEqual(print_node["label"], "printf(%d\\n)")
        self.assertEqual(
            print_node["children"][0]["label"], "42"
        )

    def test_export_does_not_execute_program(self):
        tree = self.export_ast(r"""
            int main() {
                printf("%d\n", 10 / 0);
                return 0;
            }
        """)

        labels = [node["label"] for node in self.all_nodes(tree)]

        self.assertIn("/", labels)
        self.assertEqual(tree["type"], "program")

    def test_invalid_syntax(self):
        self.assert_invalid_source("""
            int main() {
                int x = ;
                return 0;
            }
        """)

    def test_invalid_character(self):
        self.assert_invalid_source("""
            int main() {
                int x = 2 @ 3;
                return 0;
            }
        """)

    def assert_invalid_source(self, source):
        with tempfile.TemporaryDirectory() as directory:
            source_file = Path(directory) / "invalid.c"
            output_file = Path(directory) / "invalid.json"

            source_file.write_text(
                textwrap.dedent(source).strip() + "\n",
                encoding="utf-8"
            )

            result = subprocess.run(
                [str(MINIC), "--emit-json",
                 str(source_file), str(output_file)],
                capture_output=True,
                text=True,
                timeout=10
            )

            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(output_file.exists())


if __name__ == "__main__":
    unittest.main(verbosity=2)
