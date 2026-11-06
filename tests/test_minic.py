
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MINIC = ROOT / "build" / "minic"


def run_source(source):
    with tempfile.TemporaryDirectory() as directory:
        source_path = Path(directory) / "program.c"
        source_path.write_text(textwrap.dedent(source), encoding="utf-8")

        return subprocess.run(
            [str(MINIC), str(source_path)],
            capture_output=True,
            text=True,
            timeout=10
        )


class MiniCTests(unittest.TestCase):

    def test_demo_program(self):
        result = subprocess.run(
            [str(MINIC), str(ROOT / "examples" / "demo.c")],
            capture_output=True,
            text=True,
            timeout=10
        )
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout, "15\n100\n")

    def test_operator_precedence(self):
        result = run_source("""
            int main() {
                printf("%d\\n", 2 + 3 * 4);
                printf("%d\\n", (2 + 3) * 4);
                return 0;
            }
        """)
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout, "14\n20\n")

    def test_variable_scope(self):
        result = run_source("""
            int main() {
                int x = 7;
                {
                    int x = 9;
                    printf("%d\\n", x);
                }
                printf("%d\\n", x);
                return 0;
            }
        """)
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout, "9\n7\n")

    def test_short_circuit(self):
        result = run_source("""
            int main() {
                int zero = 0;
                printf("%d\\n", 0 && (10 / zero));
                printf("%d\\n", 1 || (10 / zero));
                return 0;
            }
        """)
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout, "0\n1\n")

    def test_syntax_error(self):
        result = run_source("""
            int main() {
                int x = ;
                return 0;
            }
        """)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("syntax error", result.stderr.lower())

    def test_missing_semicolon(self):
        result = run_source("""
            int main() {
                int x = 5
                return 0;
            }
        """)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("syntax error", result.stderr.lower())

    def test_division_by_zero(self):
        result = run_source("""
            int main() {
                int x = 10;
                int y = 5 - 5;
                printf("%d\\n", x / y);
                return 0;
            }
        """)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("division by zero", result.stderr)

    def test_undeclared_variable(self):
        result = run_source("""
            int main() {
                x = 10;
                return 0;
            }
        """)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("undeclared variable", result.stderr)

    def test_duplicate_declaration(self):
        result = run_source("""
            int main() {
                int x = 1;
                int x = 2;
                return 0;
            }
        """)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("duplicate declaration", result.stderr)

    def test_integer_overflow(self):
        result = run_source("""
            int main() {
                int x = 9223372036854775807;
                printf("%d\\n", x + 1);
                return 0;
            }
        """)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("integer overflow", result.stderr)

    def test_invalid_character(self):
        result = run_source("""
            int main() {
                int x = 2 @ 4;
                return 0;
            }
        """)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Unexpected character", result.stderr)

    
    def test_return_exit_code(self):
        result = run_source("""
            int main() {
                return 7;
            }
        """)

        self.assertEqual(result.returncode, 7)
        self.assertEqual(result.stdout, "")
        self.assertEqual(result.stderr, "")


if __name__ == "__main__":
    unittest.main(verbosity=2)
