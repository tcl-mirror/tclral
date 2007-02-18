if 0 {
Andrew Mangogna -- 17 Feb 2007

I've been looking at some of the new Object Oriented foundations being
put into the core for Tcl 8.5 and, naturally enough, thought about how
the new OO could be used to add behavior to '''relvars'''.
Relations provide for very powerful data structuring and manipulation,
however, there are many times when it would be convenient to tie
some application specific behavior to a relvar.

Besides, relations are an old idea and objects are a somewhat newer idea
and I'm interested to see if there is any common ground.
With new OO commands in the core, we now have a very fertile playground.
I'm looking forward to a whole new array of OO extensions
(okay, not so much, but you just know it's going to happen).

As an example, I decided to revisit [Relations as Stacks].
This example casts the ability to use relations as elementary data structures
into an object oriented form.
The details of the data structure are not what's important here.
I think the interesting part is comparing the results of the two approaches.

Needless to say, Tcl 8.5 is required.
This code was run on 8.5a5.
}

 package require Tcl 8.5
 package require ral
 namespace import ::ral::*
 package require ralutil
 namespace import ::ralutil::*

if 0 {
Like in [Relations as Stacks], we will be using a utility in '''ralutil'''
to automatically add a consecutive integer identifier.
}

 ::ralutil::sysIdsInit

if 0 {
In this design we will use a class to encapsulate relvars. There are a large
number of '''relvar''' and '''relation''' operations that could be
encapsulated as methods, but to keep the example small, I have only put
in methods that are used by the example.

Also, we use a bit of a trick here. Since '''relvar'''s have their own
namespace-like names, we can use the name of the object as the name of
the relvar.
Hence '''self''' refers to both the object command and the relvar name.
}

 oo::class create ooRelvar {
     constructor {attrs ids} {
 	relvar create [self] [list Relation $attrs $ids]
     }
     destructor {
 	relvar unset [self]
     }
     method format {args} {
 	relformat [relvar set [self]] {expand}$args
     }
     method Insert {tuple} {
 	relvar insert [self] $tuple
     }
     method Delete {args} {
 	relvar deleteone [self] {expand}$args
     }
 }

if 0 {
As discussed in [Relations as Stacks], the semantics of stacks and queues
are closely related in relational terms to the manner in which the
''last'' or ''first'' element is selected. 
Expanding on that theme here, we introduce a class that imposes an
order on a relvar. We will do that by adding an identifier consisting
of a single incrementing integer and use the helpers in '''ralutil''' to
set the identifier values.
}

 oo::class create RelOrdered {
     superclass ooRelvar
     constructor {args} {
 	# Add an attribute that will be a system generated identifier
 	lappend args __Id__ string
 	next $args __Id__
 	::ralutil::sysIdsGenSystemId [self] __Id__
     }
     destructor {
 	next
     }
     method Insert {tupleValue} {
 	# We have to make sure that the identifying attribute is
 	# mentioned, and its value will be set by a relvar trace.
 	lappend tupleValue __Id__ {}
 	next $tupleValue
     }
     method Delete {rel} {
 	next __Id__ [relation extract $rel __Id__]
     }
     method Item {rel} {
 	pipe {
 	    relation eliminate $rel __Id__ |
 	    relation tuple |
 	    tuple get
 	}
     }
     # The difference between stack semantics and queue semantics
     # is in the sorting order of the tag command. Sorting "descending"
     # selects the last item placed in the relvar. Sorting "ascending"
     # selects the first item placed in the relvar.
     method Pick {order} {
 	pipe {
 	    relvar set [self] |
 	    relation tag ~ -$order __Id__ __Order__ |
 	    relation choose ~ __Order__ 0 |
 	    relation eliminate ~ __Order__
 	}
     }
 }

if 0 {
So now we can create a stack class based on relvars.
The stack '''items''' are actually a
dictionary of attribute name / attribute value pairs.
This could be made simpler to just accept a string,
but we get a little more ''structure'' for very little additional
effort.
}

 oo::class create RelStack {
     superclass RelOrdered
     constructor {args} {
 	next {expand}$args
     }
     destructor {
 	next
     }
     method push {args} {
 	my Insert $args
	return
     }
     method pop {} {
 	set rel [my Pick descending]
 	my Delete $rel
 	my Item $rel
     }
     method peek {} {
 	my Item [my Pick descending]
     }
 }

if 0 {
The queue class is similar.
The method names are chosen to be the conventional ones.
}

 oo::class create RelQueue {
     superclass RelOrdered
     constructor {args} {
 	next {expand}$args
     }
     destructor {
 	next
     }
     method put {args} {
 	my Insert $args
	return
     }
     method get {} {
 	set rel [my Pick ascending]
 	my Delete $rel
 	my Item $rel
     }
     method peek {} {
 	my Item [my Pick ascending]
     }
 }

if 0 {
So now for simple test runs.
}

 RelStack create myStack Item string Name string

 myStack push Item "Item 1" Name A
 myStack push Item "Item 2" Name B

 puts [myStack format]
 puts "peek: [myStack peek]"
 puts "pop: [myStack pop]"

if 0 {
 +------+------+======+
 |Item  |Name  |__Id__|
 |string|string|string|
 +------+------+======+
 |Item 1|A     |1     |
 |Item 2|B     |2     |
 +------+------+======+
 peek: Item {Item 2} Name B
 pop: Item {Item 2} Name B
}

 myStack push Item "Item 3" Name C
 myStack push Item "Item 4" Name D

 puts "pop: [myStack pop]"
 puts [myStack format]
 puts "pop: [myStack pop]"

if 0 {
 pop: Item {Item 4} Name D
 +------+------+======+
 |Item  |Name  |__Id__|
 |string|string|string|
 +------+------+======+
 |Item 1|A     |1     |
 |Item 3|C     |3     |
 +------+------+======+
 pop: Item {Item 3} Name C
}

 myStack destroy

 RelQueue create myQueue Item string

 myQueue put Item "Item 1"
 myQueue put Item "Item 2"

 puts [myQueue format]
 puts "[myQueue get]"
 puts "[myQueue get]"

 myQueue destroy

if 0 {
 +------+======+
 |Item  |__Id__|
 |string|string|
 +------+======+
 |Item 1|1     |
 |Item 2|2     |
 +------+======+
 Item {Item 1}
 Item {Item 2}


So, it is possible to use relvars as the underlying data structure for
an object oriented formulation of the interfaces.
Now I'm trying to decide if I think it is a good idea or not.
It will take a good bit more thought and experimentation.
There does seem to be a fundamental tension between the usual object
oriented attempts to encapsulate data and the relational approach to
make all the data manipulation operations polymorphic with respect to
the actual relation type.
I'm also not sure where referential integrity constraints come into the
approach.
They are vitally important in relational terms, but don't seem to have
an object oriented home.

----
See also [TclRAL], [Relations as Stacks]
}
