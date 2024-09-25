# Moriarty

<p align="center">
  <img width="250px" src="logo.png" alt="Moriarty Logo">
</p>
<p align="center">
  <em>Create robust data generators and data validators with little-to-no
  boilerplate code based on just a description of the data.</em>
</p>

## What is Moriarty?

Moriarty is a tool for describing, generating, and verifying data through our
expressive syntax.

```c++
int main() {
  Moriarty M;
  M.SetName("Example Constraints")
      .AddVariable("N", MInteger(Between(1, 100)))
      .AddVariable("A", MArray<MInteger>(Elements<MInteger>(Between(3, 5)),
                                         Length("3 * N + 1")))
      .AddVariable("S", MString(Alphabet("abc"), Length("N")))
      .AddGenerator("CornerCaseGenerator", CustomCornerCaseGenerator())
      .AddGenerator("SmallExamples", SmallCaseGenerator());
  M.GenerateTestCases();
  M.ExportTestCases(SimpleIO().AddLine("N", "S").AddLine("A"));
}
```

Example output from example (where N = 2):

```
2 ab
3 4 5 4 3 4 5
```

`MVariable`s are used to describe a specific data type (primitive type, class,
struct, proto, etc.) and are written in a way to allow users to put constraints
on variables. The Moriarty team provides several `MVariable`s for common data
types but also allows experts to write their own custom types.

Users are then able to describe their variables and constraints and either:

1.  Generate and export test cases.
1.  Import and verify test cases.

We provide several defaults for generation, importing, exporting, and
verification but also allows users to customize to their heart's content!

Start <a href="docs/getting_started.md">here</a> for a tutorial walking you
through a full Moriarty example.
