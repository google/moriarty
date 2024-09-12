# MVariable Tutorial

Note: A full working version of the code for this tutorial can be found in
`examples/mexample_graph.cc`.

## Summary

An `MVariable` is a Moriarty component that an expert makes to describe a
certain object. An `MVariable` should be thought of as **a collection of
constraints**. For example, an `MInteger` describes constraints about integers
and `MString` describes constraints about strings. Examples of constraints:

*   This integer is between 1 and 10.
*   This string is of length 10 and uses English lowercase letters.
*   This account is for a user in Finland.

## Naming / Class Structure

An `MFoo` is a Moriarty component that describes constraints about `Foo`.

In general, we assume you already have (and own) a class that you would like to
describe. You will create an `MVariable` that will *describe* your class. If
your class's name is `CustomType`, then we strongly recommend your `MVariable`
be called `MCustomType`. This is to maintain a consistent and clear separation
between the original class and the Moriarty class that is describing it.

### Builder-Like Syntax

Moriarty recommends a builder-like syntax for your `MVariable`. Every call to a
function should *add a constraint* to the system. For example, the following
should have two constraints added to it:

```c++
MInteger().Between(1, 10).IsPrime();
```

If similar constraints are added multiple times, each one should be considered
as a constraint!

```c++
MInteger().Between(1, 10).AtLeast(5);      // Equivalent to Between(5, 10)
MInteger().Between(1, 10).Between(5, 20);  // Also equivalent to Between(5, 10)
```

The second example may be counterintuitive at first, but remember that we are
adding constraints to our `MInteger`: we want it to be "between 1 and 10" AND we
want it to be "between 5 and 20"---the only way for those to both be true is for
the integer to be "between 5 and 10".

### Why is `MCustomType` a separate class?

One question we receive frequently: "Why do we need a separate class for
`MCustomType`? Why can't we just extend or modify `CustomType` instead?"

There are a few reasons for this:

*   From a design perspective:
    *   The
        [single-responsibility principle](https://en.wikipedia.org/wiki/Single-responsibility_principle)
        comes into play here. `CustomType` is responsible for something
        regardless of Moriarty's existence. `MCustomType` is responsible for
        describing constraints about a `CustomType`.
*   From a technical perspective:
    *   You may not be able to change the code of `CustomType`--either you don't
        own the code or the code is auto-generated (e.g., protocol buffers).
    *   Note: If you do not own `CustomType`, carefully consider if you are the
        right person to make `MCustomType`. Moriarty thrives when experts make
        the Moriarty component, and ideally, there is exactly one Moriarty
        component for each class.

## Create your first `MVariable`

In this tutorial, we will create a Moriarty component for a
[graph](https://en.wikipedia.org/wiki/Graph_\(discrete_mathematics\)). (You
don't need to know anything about graphs other than (1) they are dots and lines
and (2) "connected" means you can walk from every dot to every other dot using 1
or more lines. We will assume an undirected graph.)

The class `ExampleGraph` is a simple implementation of a graph (nothing to do
with Moriarty, just a class that represents a graph). We will create
`MExampleGraph`, which will understand 3 types of constraints (1) number of
nodes, (2) number of edges, and (3) if the graph is connected or not. Our
constraints will look like this:

```c++
MExampleGraph G = MExampleGraph()
                    .WithNumNudes(MInteger().Is(10))
                    .WithNumEdges(MInteger().Between(10, 20))
                    .IsConnected();
```

## Step 0: Create an `ExampleGraph` class.

In normal scenarios, this class should already exist and you are just attempting
to describe it. For this tutorial, here is the `ExampleGraph` class we will use
(Note that this class does not conform to Google's C++ Style Guide, it is just
used here for simplicity).

```c++
struct ExampleGraph {
  int num_nodes;
  std::vector<std::pair<int, int>> edges; // 0-based vertex numbers.
};
```

Your type will also need three basic operators: (1) `operator<`, (2)
`operator==`, and (3) a hash function. For hashing, your type must be able to be
placed into an `absl::flat_hash_set<YourType>`.

```c++
bool operator<(const ExampleGraph& G1, const ExampleGraph& G2) {
  return std::tie(G1.num_nodes, G1.edges) < std::tie(G2.num_nodes, G2.edges);
}

bool operator==(const ExampleGraph& G1, const ExampleGraph& G2) {
  return std::tie(G1.num_nodes, G1.edges) == std::tie(G2.num_nodes, G2.edges);
}

template <typename H>
H AbslHashValue(H h, const ExampleGraph& G) {
  return H::combine(std::move(h), G.num_nodes, c.edges);
}
```

## Step 1: Create a skeleton for `MExampleGraph`

All `MVariable`s must inherit from `moriarty::librarian::MVariable<>`. [^crtp]
The template arguments passed to `MVariable<>` are (1) the Moriarty component
you are creating (`MExampleGraph`) and (2) the class you are describing
(`ExampleGraph`).

[^crtp]: `MVariable<>` uses
    [CRTP](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern).
    This is so that the parent class knows both the `CustomType` and the
    `MCustomType`.

```c++
class MExampleGraph
    : public moriarty::librarian::MVariable<MExampleGraph, ExampleGraph> {
  ...
};
```

Every `MVariable` needs several functions to be overridden:

```c++
class MExampleGraph
    : public moriarty::librarian::MVariable<MExampleGraph, ExampleGraph> {
  public:
   std::string Typename() const override { return "MExampleGraph"; }

  private:
   absl::StatusOr<ExampleGraph> GenerateImpl() override;
   absl::Status IsSatisfiedWithImpl(const ExampleGraph& g) const override;
   absl::Status MergeFromImpl(const MExampleGraph& other) override;
};
```

The meaning of these functions are hopefully reasonably straightforward:

*   `Typename` returns the name of this class (used for debugging and generic
    properties).
*   `GenerateImpl` generates a random `ExampleGraph` satisfying its constraints.
*   `IsSatisfiedWithImpl` checks if `g` satisfies all known constraints.
*   `MergeFromImpl` takes all constraints from `other` and makes this
    `MExampleGraph` also have those constraints.

Each of these `FooImpl()` functions are not directly available to users of
Moriarty. Instead, they are used in several places behind the scenes. As an
example, when a user wants a "random graph", Moriarty will use
`MExampleGraph::GenerateImpl()`--but you don't need to worry about how that
happens--it's magic!

There are other functions which can be overridden, but these are the only
required ones.

### Base Implementations

Let's make some quick implementations for each of these (we will improve them
later!).

#### GenerateImpl

```c++
// For now, we will just generate a completely random
// graph with between 5 and 10 nodes, and between 10 and 20 edges.
absl::StatusOr<ExampleGraph> MExampleGraph::GenerateImpl() {
  // RandomInteger(min, max) is a built-in function that returns an integer in
  // the inclusive range [min, max].
  int num_nodes = RandomInteger(5, 10);
  int num_edges = RandomInteger(10, 20);

  std::vector<std::pair<int, int>> edges;
  while (edges.size() < num_edges) {
    // With one parameter, RandomInteger(x) returns a random integer in the
    // semi-inclusive range [0, x).
    int u = RandomInteger(num_nodes);
    int v = RandomInteger(num_nodes);
    edges.push_back({u, v});
  }

  return ExampleGraph({.num_nodes = num_nodes, .edges = edges});
}
```

#### IsSatisfiedWithImpl

```c++
// Currently, there are *no* constraints! (we'll add some later).
// So for right now, every graph is valid.
absl::Status MExampleGraph::IsSatisfiedWithImpl(const ExampleGraph& g) const {
  // Use `absl::OkStatus()` to indicate that `g` satisfies constraints.
  return absl::OkStatus();
}
```

#### MergeFromImpl

Since there are no constraints, there is also nothing to do in MergeFrom.

```c++
absl::Status MExampleGraph::MergeFromImpl(const MExampleGraph& other) {
  return absl::OkStatus();
}
```

With this, we have a ready to use `MVariable`! (though, it doesn't do much
yet...)

## Step 2: Add Graph-Specific Functionality

Remember that we want three different constraints: (1) number of nodes, (2)
number of edges, and (3) if is it connected.

### Constraint 1: Number of Nodes

The first two functions are fairly similar: each should take in an integer which
describes how many nodes/edges we should have in our `ExampleGraph`. An
`MInteger` does exactly that -- it describes an integer!

```c++
// In mexample_graph.h:
class MExampleGraph
    : public moriarty::librarian::MVariable<MExampleGraph, ExampleGraph> {
  ...

  // Adds a constraint on the number of nodes in the graph.
  MExampleGraph& WithNumNodes(moriarty::MInteger num_nodes);

  ...
 private:
  std::optional<MInteger> num_nodes_;
};

// In m_examplegraph.cc:
MExampleGraph& MExampleGraph::WithNumNodes(moriarty::MInteger num_nodes) {
  if (num_nodes_.has_value()) {
    // If we already have constraints on the number of nodes,
    // merge these new constraints into that.
    num_nodes_->MergeFrom(num_nodes);
  } else {
    // Otherwise, these constraints are the only ones.
    num_nodes_ = num_nodes;
  }

  return *this;  // For Builder-like syntax
}
```

Note that we use `std::optional<MInteger>` in our example rather than just
`MInteger`. This is a design choice. If a user generates a value without
specifying the number of nodes, what should happen? In our case, we have decided
that generation should fail. You may choose to have defaults if nothing is
specified, but we recommend only doing this if there is a single reasonable
default.

Now that we've added a constraint, let's update the functions above:

#### GenerateImpl

```c++
absl::StatusOr<ExampleGraph> MExampleGraph::GenerateImpl() {
  // If the user has not specified number of nodes, we fail.
  if (!num_nodes_.has_value()) {
    return absl::FailedPreconditionError("Number of nodes must be constrained");
  }

  // TryRandom(variable_name, mfoo) generates a random value that satisfies all
  // of the constraints in mfoo. The variable_name can be used to help with
  // debugging on failure. Since `*num_nodes_` is an MInteger,
  // TryRandom() returns an `absl::StatusOr<int64_t>`.
  ASSIGN_OR_RETURN(int num_nodes, TryRandom("num_nodes", *num_nodes_));
  int num_edges = RandomInteger(10, 20);

  std::vector<std::pair<int, int>> edges;
  while (edges.size() < num_edges) {
    int u = RandomInteger(num_nodes); // Random int in [0, num_nodes).
    int v = RandomInteger(num_nodes);
    edges.push_back({u, v});
  }

  return ExampleGraph({.num_nodes = num_nodes, .edges = edges});
}
```

#### IsSatisfiedWithImpl

```c++
absl::Status MExampleGraph::IsSatisfiedWithImpl(const ExampleGraph& g) const {
  // The number of nodes in the graph may have a constraint on it...
  // Recall that `num_nodes_` is an `optional<MInteger>`. If there is an
  // `MInteger` there, we need to verify that `g.num_nodes` satisfies the
  // constraints inside that `MInteger`. We use
  // `SatisfiesConstraints(other_variables_constraints, value)` to check if
  // `value` is satisfies all of the constraints of the first argument.
  if (num_nodes_.has_value()) {
    RETURN_IF_ERROR(SatisfiesConstraints(*num_nodes_, g.num_nodes));
  }

  // If the number of nodes is good, then `g` satisfies constraints!
  return absl::OkStatus();
}
```

#### MergeFromImpl

```c++
absl::Status MExampleGraph::MergeFromImpl(const MExampleGraph& other) {
  // If `other` has constraints on the number of nodes, add that to our
  // constraints. Do not attempt to write special logic here, try to only
  // call public functions that correspond to the constraint.
  if (other.num_nodes_.has_value()) WithNumNodes(*other.num_nodes_);

  return absl::OkStatus();
}
```

### Constraint 2: Number of Edges

The number of edges is basically identical to the number of nodes:

```c++
// In m_examplegraph.h:
class MExampleGraph
    : public moriarty::librarian::MVariable<MExampleGraph, ExampleGraph> {
  ...

  // Adds a constraint on the number of edges in the graph.
  MExampleGraph& WithNumEdges(moriarty::MInteger num_edges);

  ...
 private:
  std::optional<MInteger> num_edges_;
};

// In m_examplegraph.cc:
MExampleGraph& MExampleGraph::WithNumEdges(moriarty::MInteger num_edges) {
  if (num_edges_.has_value()) {
    // If we already have constraints on the number of edges,
    // merge these new constraints into that.
    num_edges_->MergeFrom(num_edges);
  } else {
    // Otherwise, these constraints are the only ones.
    num_edges_ = num_edges;
  }

  return *this;  // For Builder-like syntax
}
```

#### GenerateImpl

```c++
absl::StatusOr<ExampleGraph> MExampleGraph::GenerateImpl() {
  if (!num_nodes_.has_value()) {
    return absl::FailedPreconditionError("Number of nodes must be constrained");
  }
  if (!num_edges_.has_value()) {
    return absl::FailedPreconditionError("Number of edges must be constrained");
  }

  ASSIGN_OR_RETURN(int num_nodes, TryRandom("num_nodes", *num_nodes_));
  ASSIGN_OR_RETURN(int num_edges, TryRandom("num_edges", *num_edges_));

  std::vector<std::pair<int, int>> edges;
  while (edges.size() < num_edges) {
    int u = RandomInteger(num_nodes); // Random int in [0, num_nodes).
    int v = RandomInteger(num_nodes);
    edges.push_back({u, v});
  }

  return ExampleGraph({.num_nodes = num_nodes, .edges = edges});
}
```

#### IsSatisfiedWithImpl

```c++
absl::Status MExampleGraph::IsSatisfiedWithImpl(const ExampleGraph& g) const {
  if (num_nodes_.has_value()) {
    RETURN_IF_ERROR(SatisfiesConstraints(*num_nodes_, g.num_nodes));
  }

  if (num_edges_.has_value()) {
    RETURN_IF_ERROR(SatisfiesConstraints(*num_edges_, g.edges.size()));
  }

  // Otherwise, `g` satisfies constraints!
  return absl::OkStatus();
}
```

#### MergeFromImpl

```c++
absl::Status MExampleGraph::MergeFromImpl(const MExampleGraph& other) {
  if (other.num_nodes_.has_value()) WithNumNodes(*other.num_nodes_);
  if (other.num_edges_.has_value()) WithNumEdges(*other.num_edges_);

  return absl::OkStatus();
}
```

### Constraint 3: Is Connected

For the connectivity constraint, we just need to keep a boolean value for if we
want this graph to be connected or not. Note that this constraint can only be
added (and not removed) from the graph!

```c++
// In m_examplegraph.h:
class MExampleGraph
    : public moriarty::librarian::MVariable<MExampleGraph, ExampleGraph> {
  ...

  // Adds a constraint stating that this graph must be connected.
  MExampleGraph& IsConnected();

  ...
 private:
  bool is_connected_ = false;
};

// In m_examplegraph.cc:
MExampleGraph& MExampleGraph::IsConnected() {
  is_connected_ = true;
  return *this;  // For Builder-like syntax
}
```

#### GenerateImpl

```c++
absl::StatusOr<ExampleGraph> MExampleGraph::GenerateImpl() {
  if (!num_nodes_.has_value()) {
    return absl::FailedPreconditionError("Number of nodes must be constrained");
  }
  if (!num_edges_.has_value()) {
    return absl::FailedPreconditionError("Number of edges must be constrained");
  }

  ASSIGN_OR_RETURN(int num_nodes, TryRandom("num_nodes", *num_nodes_));
  ASSIGN_OR_RETURN(int num_edges, TryRandom("num_edges", *num_edges_));

  if (is_connected_ && num_edges < num_nodes - 1) {
    // Since the graph is connected, `num_edges` should be >= `num_nodes` - 1.
    // We make a copy of `num_edges_` and add a constraint to that MInteger so
    // that we don't affect later calls to `GenerateImpl()`.
    moriarty::MInteger num_edges_copy = *num_edges_;
    num_edges_copy.AtLeast(num_nodes - 1);
    ASSIGN_OR_RETURN(int num_edges, TryRandom("num_edges", *num_edges_));
  }

  std::vector<std::pair<int, int>> edges;
  if (is_connected_) {
    // This part is not important to understanding Moriarty. We are simply
    // adding edges here to guarantee that we get a connected graph.
    for (int i = 1; i < num_nodes; i++) {
      ASSIGN_OR_RETURN(int v, RandomInteger(i));
      edges.push_back({v, i});
    }
  }

  while (edges.size() < num_edges) {
    ASSIGN_OR_RETURN(int u, RandomInteger(num_nodes));
    ASSIGN_OR_RETURN(int v, RandomInteger(num_nodes));
    edges.push_back({u, v});
  }

  return ExampleGraph({.num_nodes = num_nodes, .edges = edges});
}
```

#### IsSatisfiedWithImpl

```c++
absl::Status MExampleGraph::IsSatisfiedWithImpl(const ExampleGraph& g) const {
  if (num_nodes_.has_value()) {
    RETURN_IF_ERROR(SatisfiesConstraints(*num_nodes_, g.num_nodes));
  }

  if (num_edges_.has_value()) {
    RETURN_IF_ERROR(SatisfiesConstraints(*num_edges_, g.edges.size()));
  }

  // We use UnsatisfiedConstraintError to signal to Moriarty that some
  // constraint was not satisfied.
  if (is_connected_) {
    if (!GraphIsConnected(g)) {  // This function exists somewhere else.
      return moriarty::UnsatisfiedConstraint("G is not connected");
    }
  }

  // Otherwise, `g` satisfies constraints!
  return absl::OkStatus();
}
```

#### MergeFromImpl

```c++
absl::Status MExampleGraph::MergeFromImpl(const MExampleGraph& other) {
  if (other.num_nodes_.has_value()) WithNumNodes(*other.num_nodes_);
  if (other.num_edges_.has_value()) WithNumEdges(*other.num_edges_);
  if (other.is_connected_) IsConnected();

  return absl::OkStatus();
}
```

### Future Work

Both `WithNumNodes` and `WithNumEdges` should probably have an overload which
takes just an `int` (so the user can write `WithNumNodes(10)`) as well as
optionally a two parameter version and versions that take strings so that your
variable can interact with other variables (e.g., `WithNumNodes("3 * N + 1")`
and `WithNumEdges(1, "N")`.) These are outside the scope of this tutorial, but
these functions are simple to write: they should create an `MInteger` with the
appropriate parameters and pass it to the `WithNumNodes`/`WithNumEdges` we made
above.

## Conclusion

Voila! You have a fully functional Moriarty type. Now other Moriarty users can
use `MExampleGraph` to create and validate a `ExampleGraph`:

```c++
ExampleGraph G = Random(MExampleGraph()
                          .WithNumNodes(MInteger().Between(1, 10))
                          .WithNumEdges(MInteger().Between(20, 30))
                          .IsConnected());
```
