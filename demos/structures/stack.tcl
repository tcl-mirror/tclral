if 0 {

Andrew Mangogna  10 Feb 2007

Since relation values can be used to define any data structure,
this page shows some examples of defining common data structures using
relations.
Now, just because you can doesn't always mean you should.
More on this later, but these examples are really just meant to
show what is possible.
I'm not ready yet to replace everything with relations. Soon!

----

The first example will be of using a relation as a stack.
}

    package require ral
    namespace import ::ral::*
    # You will need Version 0.8.1 of TclRAL to get some utility procs
    package require ralutil
    namespace import ::ralutil::*
  
if 0 {
We need a relvar to hold the items we intend to stack.
That relvar needs an identifier and we will use a simple string attribute.
The identifier is arbitrary, so we will use consecutive numbers.
}

    relvar create Stack {
      Relation
      {Id int Item string}
      Id
    }
    ::ralutil::sysIdsInit
    ::ralutil::sysIdsGenSystemId Stack Id

if 0 {
As of Version 0.8.1 of TclRAL, you can trace relvar operations.
The ''ralutil'' package includes some procs to create consecutive
integer identifers automatically using tracing of relvar insert
operations.
So we will use that here so we don't have to get involved in
assigning identifiers.
If you are interested in the details,
the '''ralutil''' code for this is not large.
Now every attempt to insert a tuple into '''Stack''' will cause the
'''Id''' to be assigned the next consecutive integer.

Another useful utility provided by '''ralutil''' is a command pipe.
This idea the the same as [Commands pipe] although the '''ralutil'''
implementation is a bit different.
It is often the case in relational expressions that one result is
fed immediately into another operation.
It does not take very many of these for the nesting to be difficult
to read.
And since this is Tcl, we can have our own control structures.
So '''pipe''' separates commands with a vertical bar (|) and feeds
the result of one command to the subsequent command.
The tilde (~) is used to show where in the argument list the previous
result belongs and if the ~ is missing then the result is appended
to the command.
}

    proc last {} {
	set lastStacked [pipe {
	    relvar set Stack |
	    relation tag ~ -descending Id __Order__ |
	    relation choose ~ __Order__ 0
	}]
	if {[relation isempty $lastStacked]} {
	    error "stack underflow"
	}
	return $lastStacked
    }

if 0 {
This proc is the heart of the stack.
It chooses the last item put on the stack.
It accomplishes this by '''tag'''ing the relation value held
in '''Stack'''.
The '''relation tag''' command adds an attribute that is
a sequential integer and can do so in a sorted order.
So, since the '''Id''' attribute is a set of increasing integers
we can tag the value in decreasing order and then select
the one tuple that has '''__Order__''' equal to 0.
This gives us the tuple with the largest value of '''Id''' which
is the tuple that was last ''pushed'' onto the stack.
The '''last''' proc returns a relation value containing a single
tuple that is the tuple last inserted into the '''Stack''' relvar.

The typical stack operations are then quite small to code.
}

    proc push {item} {
	relvar insert Stack [list Id {} Item $item]
    }

if 0 {
Since we have a trace on the '''relvar insert''', the Id attribute
will be assigned an increasing integer number and we don't have
to supply a value in the insert.
}

    proc pop {} {
	relation assign [last]
	relvar deleteone Stack Id $Id
	return $Item
    }

if 0 {
The '''relation assign''' command takes a relation value of cardinality 1
and assigns the values of the attributes to Tcl variables that have
the same name as the attribute (think '''lassign''').
}

    proc peek {} {
	return [relation extract [last] Item]
    }

if 0 {
The '''relation extract''' command takes a relation value of cardinality 1
and returns the value of one or more of the attributes.
In this case we are only interested in the '''Item''' attribute.

Running some small tests we get:
}

    push "Item 1"
    push "Item 2"
    puts [relformat $Stack]

if 0 {
 +===+------+
 |Id |Item  |
 |int|string|
 +===+------+
 |1  |Item 1|
 |2  |Item 2|
 +===+------+
}

puts [peek]
puts [pop]

if 0 {
 Item 2
 Item 2
}

push "Item 3"
puts [relformat $Stack]

if 0 {
 +===+------+
 |Id |Item  |
 |int|string|
 +===+------+
 |1  |Item 1|
 |3  |Item 3|
 +===+------+
}

puts [pop]
puts [pop]
catch {pop}
puts $::errorInfo

if 0 {
 Item 3
 Item 1
 stack underflow
     while executing
 "error "stack underflow""
     (procedure "last" line 8)
     invoked from within
 "last"
     (procedure "pop" line 2)
     invoked from within
 "pop"
     invoked from within
 "puts [pop]"
}

if 0 {
So indeed it is straight forward to create a stack using relations.
It is interesting to note that if we modify the '''last''' proc
so that '''Stack''' is tagged in ''-ascending'' order on '''Id''',
then what we get is queue behavior, i.e. '''last''' becomes '''first'''.

Of course, this can be generalized into:
}

    proc pick {relvarName attrName direction} {
	return [pipe {
	    relvar set $relvarName |
	    relation tag ~ -$direction $attrName __Order__ |
	    relation choose ~ __Order__ 0 |
	    relation eliminate ~ __Order__
	}]
    }

    push "Item 1"
    push "Item 2"
    push "Item 3"
    push "Item 4"
    puts [relformat $Stack Stack]
    puts [relformat [pick Stack Id descending] "last item pushed"]
    puts [relformat [pick Stack Id ascending] "first item pushed"]

if 0 {
 +===+------+
 |Id |Item  |
 |int|string|
 +===+------+
 |4  |Item 1|
 |5  |Item 2|
 |6  |Item 3|
 |7  |Item 4|
 +===+------+
 Stack
 -----
 +===+------+
 |Id |Item  |
 |int|string|
 +===+------+
 |7  |Item 4|
 +===+------+
 last item pushed
 ----------------
 +===+------+
 |Id |Item  |
 |int|string|
 +===+------+
 |4  |Item 1|
 +===+------+
 first item pushed
 -----------------

Interestingly, this works for any relvar that has an attribute that
has an ordering on it.

So, am I ready to throw away '''struct::stack''' and '''struct::queue'''.
By no means!

If the problem I'm solving is dominated by stack-ness or queue-ness,
then choosing a traditional stack or queue data structure makes sense.
But in many cases the stack or queue nature is a minor aspect of
the problem and sometimes you are only intested in the first or last
think inserted in a relvar during development or debugging.
In those cases, being able to obtain stack or queue behavior from
a relation value without having to manage another data structure on the
side can be quite convenient.
}

if 0 {
----
[Category Data Structure]
}
