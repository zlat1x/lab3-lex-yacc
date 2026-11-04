
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
MINIC = ROOT / "build" / "minic"


class CodeGenerationTests(unittest.TestCase):

    def compare_programs(self, source, expected_output=None, expected_error=None):
        with tempfile.TemporaryDirectory() as directory:
            source_file = Path(directory) / "input.c"
            python_file = Path(directory) / "output.py"

            source_file.write_text(
                textwrap.dedent(source).strip() + "\n",
                encoding="utf-8"
            )

            generation = subprocess.run(
                [str(MINIC), "--emit-python",
                 str(source_file), str(python_file)],
                capture_output=True,
                text=True,
                timeout=10
            )

            self.assertEqual(generation.returncode, 0, generation.stderr)
            self.assertTrue(python_file.is_file())

            original = subprocess.run(
                [str(MINIC), str(source_file)],
                capture_output=True,
                text=True,
                timeout=10
            )

            translated = subprocess.run(
                ["python3", str(python_file)],
                capture_output=True,
                text=True,
                timeout=10
            )

            self.assertEqual(original.returncode, translated.returncode)
            self.assertEqual(original.stdout, translated.stdout)

            if expected_output is not None:
                self.assertEqual(original.returncode, 0, original.stderr)
                self.assertEqual(original.stdout, expected_output)

            if expected_error is not None:
                self.assertNotEqual(original.returncode, 0)
                self.assertIn(expected_error, original.stderr)
                self.assertIn(expected_error, translated.stderr)

    def test_precedence(self):
        self.compare_programs("""
            int main() {
                printf("%d\\n", 2 + 3 * 4);
                printf("%d\\n", (2 + 3) * 4);
                return 0;
            }
        """, expected_output="14\n20\n")

    def test_nested_scopes(self):
        self.compare_programs("""
            int main() {
                int x = 7;
                {
                    int x = 9;
                    printf("%d\\n", x);
                }
                printf("%d\\n", x);
                return 0;
            }
        """, expected_output="9\n7\n")

    def test_short_circuit(self):
        self.compare_programs("""
            int main() {
                int zero = 0;
                printf("%d\\n", 0 && (10 / zero));
                printf("%d\\n", 1 || (10 / zero));
                return 0;
            }
        """, expected_output="0\n1\n")

    def test_signed_division_and_remainder(self):
        self.compare_programs("""
            int main() {
                printf("%d\\n", -7 / 3);
                printf("%d\\n", -7 % 3);
                return 0;
            }
        """, expected_output="-2\n-1\n")

    def test_printf_without_newline(self):
        self.compare_programs("""
            int main() {
                printf("%d", 2);
                printf("%d\\n", 3);
                return 0;
            }
        """, expected_output="23\n")

    def test_while_and_if(self):
        self.compare_programs("""
            int main() {
                int sum = 0;
                int i = 1;

                while (i <= 4) {
                    sum = sum + i;
                    i = i + 1;
                }

                printf("%d\\n", sum);

                if (sum == 10) {
                    printf("%d\\n", 1);
                } else {
                    printf("%d\\n", 0);
                }

                return 0;
            }
        """, expected_output="10\n1\n")

    def test_division_by_zero(self):
        self.compare_programs("""
            int main() {
                int zero = 0;
                printf("%d\\n", 10 / zero);
                return 0;
            }
        """, expected_error="division by zero")

    def test_integer_overflow(self):
        self.compare_programs("""
            int main() {
                int x = 9223372036854775807;
                printf("%d\\n", x + 1);
                return 0;
            }
        """, expected_error="integer overflow")


if __name__ == "__main__":
    unittest.main(verbosity=2)
