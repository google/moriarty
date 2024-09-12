# Getting Started

This tutorial will take you through the process of generating some data and
printing it to the screen.

In general, there are three steps to any Moriarty program:

1.  Describe the data,
1.  Get the data into the system (import or generate), and
1.  Do something with the data (export or analyze).

See `examples/example_graph_main.cc` for a full working program which generates
a random graph.

## Step 1: Describe the Data

"Data" refers to a collection of variables with certain restrictions. The
variables can be common types (e.g., integers, strings, arrays of variables) or
custom types (e.g., graphs, billing accounts, usage metrics).

What constraints you can specify on each variable depends on the type. Here are
some examples for common data types:

*   Integers: you can specify bounds ("between 1 and 10") or specify properties
    ("a prime number" or "multiple of 5").
*   Strings: you can specify the length of the string ("length between 3 and
    50"), the characters to be used in the string ("the string should be only
    zeroes and ones"), or restrictions about the frequency of letters ("there
    should be no repeated characters in the string").
*   Arrays: you can specify the size of the array ("there should be between 6
    and 12 entries in the array") or the order of the array ("the elements
    should be sorted").

For custom types, the constraints are custom made for the type. For example, a
billing account could be a "privileged" account or a "large" account. The
meaning of those is very situation specific, but should make sense in the
context of that billing account class.

With Moriarty, you convey all constraints using a builder-like syntax:

```c++
// Example snippets:
MInteger().Between(1, 100).MultipleOf(4);
MString().OfLength(7).WithAlphabet("ACGT");
MBillingAccount().WithSize("large");
```

We're now ready to give this information to Moriarty:

```c++
int main() {
  Moriarty M;
  M.AddVariable("X", MInteger().Between(1, 100).MultipleOf(4));
  M.AddVariable("Y", MString().OfLength(7).WithAlphabet("acgt"));
  M.AddVariable("Z", MBillingAccount().WithSize("large"));
}
```

At this point, your program does not do anything. You have described your data
to Moriarty. It will use that information later when generating, importing,
exporting, and analyzing.

### Interacting Variables

Variables in Moriarty do not live in a silo. Each variable can have context of
other variables. In the previous example, we can set the length of `Y` to be the
value of `X`.

```c++
int main() {
  Moriarty M;
  M.AddVariable("X", MInteger().Between(1, 100).MultipleOf(4));
  M.AddVariable("Y", MString().OfLength("X").WithAlphabet("acgt"));
  M.AddVariable("Z", MBillingAccount().WithSize("large"));
}
```

And we're not limited there, we can also use mathematical expressions:

```c++
int main() {
  Moriarty M;
  M.AddVariable("X", MInteger().Between(1, 100).MultipleOf(4));
  M.AddVariable("Y", MString().OfLength("3 * X + 1").WithAlphabet("acgt"));
  M.AddVariable("Z", MBillingAccount().WithSize("large"));
}
```

In the examples below, we will simply generate an integer and a string and
ignore the billing account.

## Step 2: Generate the Data

In order to generate data, you must write a class which extends Moriarty's
`Generator` class and tell Moriarty about it.

```c++
class MyFancyGenerator : public Generator {
  ...
};

int main() {
  Moriarty M;
  M.AddVariable( ... );

  M.AddGenerator("Name of my generator", MyFancyGenerator());
}
```

The only function that needs to be overridden in your generator is the
`GenerateTestCases()` function. You will make several calls to `AddTestCase()`,
which returns a reference to a newly generated test case. This example generates
4 different test cases.

```c++
class MyFancyGenerator : public Generator {
 public:
  void GenerateTestCases() override {
    // You can set the variables directly to a value:
    AddTestCase().SetVariable("X", 2).SetVariable("Y", "gattaca");

    // You can add more constraints to the variables:
    AddTestCase()
      .SetVariable("X", 2)
      .SetVariable("Y", MString().WithAlphabet("a"));

    // You can choose to only specify a subset of the variables, the rest will
    // be randomly generated.
    AddTestCase().SetVariable("X", 2);

    // Similarly, a naked call to AddTestCase() will set all variables randomly.
    AddTestCase();
  }
};
```

Need some randomness? No problem! You can get random values via the `Random()`
method. For example, say you need a binary string.
`Random(MString().WithAlphabet("01").OfLength(5))` will generate one for you.
Any type that is supported in Moriarty can be randomly generated in this way
from inside a Generator.

```c++
class MyFancyGenerator : public Generator {
 public:
  void GenerateTestCases() override {
    // Create a random integer between 1 and 10.
    int64_t n = Random(MInteger().Between(1, 10));

    // Create a random string of 'a', 'c', 'g', then we'll append a 't' to it.
    std::string s = Random(MString().OfLength(6).WithAlphabet("acg"));
    s += "t";  // Append a 't'

    AddTestCase().SetVariable("X", 4 * n).SetVariable("Y", s);
  }
};
```

### Global vs Local Constraints

There are two different type of constraints:

*   "Global constraints" are the ones specified directly to Moriarty.
*   "Local constraints" are the ones specified to each `AddTestCase()`.

When generating, **Moriarty will merge these two constraints together**. This
allows you to only specify your global constraints once, and they are applied
everywhere.

```c++
class PrimeGenerator : public Generator {
 public:
  void GenerateTestCases() override {
    // This is a local constraint. This will generate a prime number between
    // 1 and 10 (since "Between(1, 10)" is a global constraint).
    AddTestCase().SetVariable("X", MInteger().IsPrime());
  }
};

int main() {
  Moriarty M;
  M.AddVariable("X", MInteger().Between(1, 10)); // Global constraint
}
```

This is true even when you specify local constraints which conflict with the
global constraints:

```c++
class ConstraintsAreMerged : public Generator {
 public:
  void GenerateTestCases() override {
    // This is a local constraint, and it will be merged with the
    // global constraint, so even though you want X to be an integer between
    // 5 and 20 here, it will generate an integer between 5 and 10 so that it
    // satisfies both global and local constraints.
    AddTestCase().SetVariable("X", MInteger().Between(5, 20));
  }
};

int main() {
  Moriarty M;
  M.AddVariable("X", MInteger().Between(1, 10)); // Global constraint
}
```

## Step 3: Export the Data

Exporting the data is similar to Generating the data. You must write a class
which extends `Exporter` and tell Moriarty about it.

```c++
class MyFirstExporter : public Exporter {
  ...
};

int main() {
  Moriarty M;
  M.AddVariable( ... );
  M.AddGenerator( ... );

  M.ExportTestCases(MyFirstExporter());
}
```

There are several functions you may override in an exporter, but only
`ExportTestCase()` is required. `ExportTestCase` will be called once for every
test case, in order. To access the value of a variable, use `GetValue`.

```c++
class MyFirstExporter : public Exporter {
 public:
  void ExportTestCase() override {
    int x = GetValue<MInteger>("X");
    std::string y = GetValue<MString>("Y");

    // Simply print to screen
    std::cout << x << " " << y << std::endl;
  }
};
```

That's it! Moriarty will call `ExportTestCase()` once for every test case. If
you would like to do things before, between, or after test cases, you can also
override `StartExport`, `TestCaseDivider`, and `EndExport`.

### Putting It All Together

We can put all three sections together into one program now!

```c++
class MyFancyGenerator : public Generator {
 public:
  void GenerateTestCases() override {
    AddTestCase().SetVariable("X", 2).SetVariable("Y", "gattaca");

    AddTestCase()
      .SetVariable("X", 10)
      .SetVariable("Y", MString().WithAlphabet("a"));
  }
};

class MyFirstExporter : public Exporter {
 public:
  void ExportTestCase() override {
    int x = GetValue<MInteger>("X");
    std::string y = GetValue<MString>("Y");

    std::cout << x << " " << y << std::endl;
  }
};

int main() {
  Moriarty M;
  M.AddVariable("X", MInteger().Between(1, 100).MultipleOf(4));
  M.AddVariable("Y", MString().OfLength("3 * X + 1").WithAlphabet("acgt"));

  M.AddGenerator("Fancy Generator", MyFancyGenerator());

  M.ExportTestCases(MyFirstExporter());
}
```
