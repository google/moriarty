# Moriarty

<center>
  <div style="margin:auto; max-width:400px;">
    <img width="250px" src="logo.png">
    <br/>
    <p style="text-align:center; font-style: italic;">
      Create robust data generators and data validators with little-to-no
      boilerplate code based on just a description of the data.
    </p>
  </div>
</center>

## What is Moriarty?

Moriarty is a tool for describing, generating, and verifying data through our
expressive syntax.

```c++
int main() {
  Moriarty M;
  M.SetName("Example Constraints")
      .AddVariable("N", MInteger().Between(1, 100))
      .AddVariable("A", MArray(MInteger().Between(3, 5)).OfLength("3 * N + 1"))
      .AddVariable("S", MString().WithAlphabet("abc").OfLength("N"))
      .AddGenerator("CornerCaseGenerator", CustomCornerCaseGenerator())
      .AddGenerator("SmallExamples", SmallCaseGenerator());
  M.GenerateTestCases();
  M.ExportTestCases(SimpleIO().AddLine("N", "S").AddLine("A"));
}
```

Example output from example:

```
2 ab
3 4 5 4 3 4 5
```

`MVariable`'s are used to describe a specific data type (primitive type, class,
struct, proto, etc.) and are written in a way to allow users to put constraints
on variables. The Moriarty team provides several `MVariables` for common data
types but also allows experts to write their own custom types.

Users are then able to describe their variables and constraints and either:

1.  Generate and export test cases.
1.  Import and verify test cases.

We provide several defaults for generation, importing, exporting, and
verification but also allows users to customize to their heart's content!

Start <a href="docs/getting_started.md">here</a> for a tutorial walking you through a full Moriarty example.
