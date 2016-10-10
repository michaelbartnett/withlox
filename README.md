
[//]: # -*- fill-column: 80  -*-

## Current TODO

This will be maintained. Latest log entry will go below.

- [ ] Milestone: Solid GUI Terminal: Test/develop on Windows

- [X] CLI command to load a directory of JSON files into a collection DynArray<Value>

- [X] CLI command to show an editing window for the value collection

- [ ] CLI command to save the DynArray<Value> back out to JSON files

- [X] The `loadjson` command should store the `Value`s in a collection
     tracking both index and load path parameter.

- [X] An`edit` command that will focus/create an editor window for the
     specified collection (could be index or the load path)

- [X] A `drop` command that will remove the specified collection from memory

- [ ] Upgrade dearimgui

- [ ] Fix the text field string length problem

- [ ] TypeDescriptor list window, show all or for collection

## 2016-10-10-0304EST Dropping collections works

Dropping collections works. I had to add more operations for
bucketarray and dynarray that revolve around getting indexes for
values and pointers and checking ranges.

I also moved all the bucketarray and dynarray operations into
namespaces rather than prefix every thing with `bucketarray_` or
`dynarray_`. This is how bitsquid does it, and it makes more sense to
me (and looks nicer, IMO). Later I'll do the same thing for the
hashtable, but that thing is complicated, and it has a weird name.

I think a window that I want to build next is one that can list all of
the type descriptors in a given collection, as well as all the
typedescriptors allocated globally (including those not referencing
any values).

- [] TypeDescriptor list window, show all or for collection

But before that I'm going to upgrade dearimgui.

I also fucked around with tracing allocator calls. It was somewhat
educational.

## 2016-10-09-0413EST Log UI truncation works

I was able to greatly simplify the `concatenated_log` function by just
re-writing the whole string whenever it changes on a frame.

There's an optimization loss here when the log length is smaller than
`STR_LENGTH_MAX`, but that's probably gonna be rare, so w/e.

## 2016-10-06-0429EST Really beating on it

While making the multi-collection-editor commands I described in the
last entry, I decided to grab some more test data. I found this
community game on github called "Journey to the Center of Hawkthorne".

http://projecthawkthorne.com/

https://github.com/hawkthorne/hawkthorne-journey

I chose it because they have the json files describing characters that
look exactly like the kind of data I want this thing to be good at
editing, and they have a clearly written LICENSING.md file that states
that the the code is licensed under an MIT-like license, and the
assets are all under a Creative Commons BY-NA license. I don't think I plan
on selling this tool, so that works perfectly for me.

I ran into a few issues, one terrifying, and a couple of others that I
expected to have sooner or later.

The terrifying one was a bug in `dynarray_copy`. Pretty bad if that's broken.

It came in two parts. I'll include diffs so you can see how bad I am.

The first is that the counts were reversed here.

```diff
 void dynarray_copy(DynArray<T> *dest, const DynArray<T> *src)
 {
     assert(src->count <= dest->capacity);
-    dynarray_copy(dest, 0, src, 0, dest->count);
+    dynarray_copy(dest, 0, src, 0, src->count);
 }
```

You want to copy the `src` count of items to the `dest`, and not the
other way around, because this is most commonly called from
`dynarray_clone` which initializes the `dest` array and then copies
the `src` contents into it via this function. So `dest->count` is
usually 0!

The second part is really bad:

```diff
 template<typename T>
 void dynarray_copy(DynArray<T> *dest, DynArrayCount dest_start, const DynArray<T> *src, DynArrayCount src_start, DynArrayCount count)
 {
     assert(src_start + count <= src->count);

     DynArrayCount final_count = max<DynArrayCount>(dest_start + count, dest->count);
     assert(final_count > dest_start || count == 0);

     DynArrayCount capacity_required = dest_start + count;
     dynarray_ensure_capacity(dest, capacity_required);
-    std::memcpy(dest + dest_start, src + src_start, count * sizeof(T));
+    std::memcpy(dest->data + dest_start, src->data + src_start, count * sizeof(T));
     dest->count = final_count;
 }
 ```

This was doing a memcpy of the container directly itself rather than
just the `data` field. That in and of itself is not so bad, it's sort
of like assigning one to the other. EXCEPT: the `count * sizeof(T)`
expression. If my arrays were larger this would have been spotted
faster because it would have written into protected memory and
faulted. That would have been much nicer. Instead I got an infinite
loop in some other part of the code.

The other issue has to do with the size of character buffers. This
first manifested as an assert firing in FormatBuffer stating that its
capacity had exceeded the max (UINT16_MAX, specifically). With my
confidence shattered into pieces from the last bug, I figued that I
had better make sure that FormatBuffer wasn't also broken.

Fortunately, it mostly wasn't. It was kinda bad, but not broken. What
would happen is that it would keep trying to double size when it ran
out of room to write more chars. Problem is that eventually it would
reach close to the max, like 61k, and then decide to double.

Okay, not broke, but clearly behavior could be better. So I put the
max capacity asserts immediately where RESIZE_ARRAY calls happen, and
when doubling the size, I clamp it to the max capacity.

Here's the capacity doubling code (in `ensure_formatbuffer_space`):

```diff
-        fb->capacity = (fb->capacity * 2 < fb->capacity + length + 1
-                        ? fb->capacity + length + 1
-                        : fb->capacity * 2);
-        RESIZE_ARRAY(fb->allocator, fb->buffer, fb->capacity, char);
+        size_t new_capacity = min(FormatBuffer::MaxCapacity - 1,
+                                  (fb->capacity * 2 < fb->capacity + length + 1
+                                   ? fb->capacity + length + 1
+                                   : fb->capacity * 2));
+
+        RESIZE_ARRAY(fb->allocator, fb->buffer, new_capacity, char);
+        fb->capacity = new_capacity;
```

And here's the check at the beginning of that function with an assert added:

```diff
 static void ensure_formatbuffer_space(FormatBuffer *fb, size_t length)
 {
     size_t bytes_remaining = fb->capacity - fb->cursor;

+    assert((fb->capacity - bytes_remaining + length) < FormatBuffer::MaxCapacity);
```

So when I tested again, I got the same asserts firing, but with a different callstack.

In the command function where this was being called (`loadjson`) it
was accumulating output in the format buffer and not flushing it until
after looping through all that it wanted to loop through. Easy fix:
flush at the end of the loop body. 

Now there are no more asserts firing in FormatBuffer routines. I don't
know if this is good behavior long term. I could see arguments for
keeping it as-is and expecting code just flush enough, but I could
also see arguments for FormatBuffer being able to handle as many chars
as there is virtual memory to go around. I'll table that for now.

But now there's one last assert: when drawing the console log to IMGUI
I take an accumulated list of log entry string and concat them
together for display. This uses my `Str` type which has the same
UINT16_MAX length limit. Maybe these narrow size fields are a waste of
time, but I think they keep me hoenst. I haven't done it yet, but the
fix I think I'll apply here is to just drop old entries.

But the most reasonable way to do that is by having a ringbuffer-style
queue, and I don't have one of those yet...

Actually that doesn't even work, because IMGUI expects the char buffer
to be continuous! Ack! I have to sleep on this.

Oh! Also, can I just say that my integral type conversion functions are
the best?

This is how I got the Str length assert:

```
min_capacity = STRLEN(min_capacity + log_entries[i].length + 1);
```

That `STRLEN` macro expands to a call to an overloaded function that
converts `size_t` to `u16` and asserts that the input is in a valid
range.

Alright, I'm out.

# 2016-10-03-0429EST TypeRefs are no more

In a flurry of furious refactoring I got rid of TypeRef. It was easier
than I thought it was going to be. I started by adding
`TypeDescriptor*` versions of fields to structs in `types.h` and
implementing overloaded or alternately-named versions of a few key
leaf functions, such as `add_typedescriptor`.

Then one by one I'd start removing the `TypeRef` fields. This would
trigger compiler errors in various functions. These functions would
often take `TypeRef`s as parameters, so in those cases I would make a
duplicate stub function that did a nominal amount of work, possibly
referencing `TypeRef` or other fields so I would be prompted to delete
the stubs later once I finally deleted the `TypeRef` struct
itself. The actual function's parameters were then changed to take
`TypeDescriptor*`s, and this would trigger further compiler errors
which would help guide me through making appropriate changes in the
function.

There was one small bug, but it was so small I don't even remember
what it was. There was no issue with BucketArray since I had already
tested it.

I also imported my numeric conversion functions from my
raytracer/blender importer project. This is a big ugly macro mess that
defines conversion functions for all unsigned and signed explicitly
sized integral types, plus `size_t` and `intptr_t`. I tried templates
at first, but I realized that in order to do this effectively you
probably want something like extern template that can restrict what
instantiations are allowed without having to write out each
implementation.

There's this todo item:

- CLI command to show an editing window for the value collection

I think I have a clearer idea on what I can do to make progress, so
I'm replacing it with new todo items. I basically want the `loadjson`
command to make some sort of `Collection` record containing the array
of `Value`s. The collection editor shouldn't pop up by default, I want
to be able to list loaded collections, and then have a separate
command to open or focus an editor window displaying those values.

And then, to get the ball rolling on UI, I want to upgrade IMGUI and
fix the string length problem.

- [X] The `loadjson` command should store the `Value`s in a collection
     tracking both index and load path parameter.

- [X] An`edit` command that will focus/create an editor window for the
     specified collection (could be index or the load path)

- [X] A `drop` command that will remove the specified collection from memory

- [] Upgrade dearimgui

- [] Fix the text field string length problem

It has also become clear that I need to think about iteration mechanisms for
the hashtable and bucketarray.

# 2016-10-03-0256EST BucketArray

I called it BucketArray, because instead of doing a fancy slot map or
plf::colony thing I just reimplemented what Jonathan Blow showed off
in his polymorphic struct demo (polymorphic struct means class
template in C++ish).

It's not total amazeballs, but it does what I wanted, which it to
be a pretty okay container for all my TypeDescriptor objects and
does not relocate anything.

Now I should go and get rid of TypeRef and favor TypeDescriptor
pointers everywhere.

While I'm doing this, it's a good opportunity to take stock of the
type manipulation code. I improved it slightly when I added the type
merging functions, but it's still not super well organized, in large
part because `main()` has these big functions that infer types from
imported JSON. Seems like that should probably be a separate
module/header.

After that I should really get back to trying to make UI (this is
supposed to be a visual data editor, RIGHT??). The major outstanding
problem from last time was that I was not allocating enough scratch
space for dearimgui's text editing widget.

I recall that before I was trying to think about expanding data fields
inline or something but that's obviously wrong.

So I should upgrade dearimgui (lots of upstream changes for the
better, including my OSX input changes!
https://github.com/ocornut/imgui/pull/650 :D).

# 2016-10-01-0447EST Just malloc

In order to make progress on getting rid of TypeRef, I've decided to
forego custom storage for auxiliary TypeDescriptor data for the time
being. CompoundType and UnionType will use the default DynAarray
allocator as they do now, and then I'll figure out the right strategy
for them later.

In the meantime, I'm going to build a ChunkedArray<T> that will act
sort of like a std::deque or a plf::colony (see CppCon 2016 talk).

Maybe I'll name it ChunkChainArray, ArrayChain, ChainArray...idunno.

# 2016-09-028-1237EST Storing auxiliary TypeDescriptor data

To restate the problem from before:

> UnionType has a DynArray<TypeDescriptor *> and CompoundType has a
> DynArray<CompoundTypeMember>, where each CompoundTypeMember has two
> pointers.

It's pretty clear that I need to have a separate memory pool for auxiliary
TypeDescriptor data that allocates up to an alignment of `sizeof(void *[2])`.

This is a little annoying, because I want to build up these types
iteratively in the JSON/type description parser, so the dynamic resize
behavior of DynArray<T> is desirable. But I really don't want to be
constantly allocating and deallocating from this memory pool, because
that sounds prone to fragmentation.

That can just be solved by having some scratchpad memory though, right?
And when I've finished deciding all the types that go into a Union, I can
do some sort of bake/cementing operation on a TypeDescriptor which copies
its auxiliary data to permanent storage.

The storage for auxiliary TypeDescriptor data needs to be able to
handle arbitrary multiple of `sizeof(void *[2])`, which sort of makes
it sound like I'm just reimplementing malloc. At this point it's
sensible to ask: why am I even bothering?

Maybe it's not worth bothering with, but the hope is that my allocator
can go a bit faster than plain malloc because my allocator wouldn't
need to be as generic. It's also good to be able to track allocator
stats and debug leaks by going through my allocator infrastructure.

If I continue to take inspiration for my allocator interfaces after
Alexandrescu's talks, then I'll probably be in an okay place.

The TypeDescriptors definitely need special storage, because I probably
want to garbage collect them.

Some notes on that, by the way. I probably want a sesion-scoped
identifier for each TypeDescriptor because if they're sequential then
I can use bitsets or nibble or byte arrays to track meta data (such as
GC markers) and if I do ever want to compact memory, I could use this
identifier for pointer patching.

# 2016-09-28-0425EST Coming Back

I'm looking at this again because I have to deal with more spreadsheet
nonsense at work so now I'm pining for a finished version of this tool
again.

I watched Andrei Alexandrescu's allocator talk again, and started
looking at his design for allocators in D, which mimics what he
presented for C++.

Looking at the code, I think the whole TypeRef strategy is probably a
little misguided. There should be chunked array storage for
TypeDefinitions, and there's not really a reason to use handles as far
as I can see. When is a TypeDescriptor freed? It only ever makes sense
to free it when it's no longer in use, but it's not a requirement to
do so, especially since you want to be able to load and unload data
multiple times and use the same type descriptors. No use recreating them
all over again.

Garbage collecting TypeDescriptors can happen when context severely
changes (e.g. user closes a lot of open data files) or--if memory
becomes an issue--I can actually do a mark-and-sweep GC pass on
TypeDescriptors.

I mean really, look at the definition of TypeRef:

```
struct TypeRef
{
    u32 index;
    DynArray<TypeDescriptor> *owner;
};
```

Pointer to a DynArray and an index. It's stupid, just don't deallocate
slabs of TypeDescriptors. It should look like the chunked array data
structure that Jonathan Blow demoed when he was showing off integral
template parameters in Jai.

For Values, the same thing should happen. These should be much easier
to clean up though, since the data being edited in this tool is
intended to be mostly hierarchical. I will probably want to add a
referencing capability at some point, but I'll cross that bridge when
I come to it.

Now that I think about it more though, not everything perfectly fits
into a chunk/pool heap like I was thinking of, because CompoundType
and Union type have dynamic elements.

UnionType has a DynArray<TypeDescriptor *> and CompoundType has a
DynArray<CompoundTypeMember>, where each CompoundTypeMember has two
pointers.

# 2016-06-22-0208EST Set Phasers to Disjoint

So I implemented `merge_types`, `compound_member_merge`, as well as a
`make_union` support function I used to write `merge_types` and holy
cow `make_union` is the longest and most horrible.

I threw a few comments in the code amounting to "wow a Set container
would be nice" and "at least give me `dynarray_add_to_set`". It's
really pretty rough not having set operations for when dealing with
merging unions.

It would also probably make `compound_member_merge` cleaner too, if I
template it correctly.

I ended up writing a `free_typedescriptor_components` function because
it was bothering me that I would speculatively create all these type
descriptors with member and type case arrays, only to let those arrays
leak. This isn't *that* great (memory management of core types needs
work in general), but at least now it leaks less.

Something I didn't cleanup was `NameRef` values. I might never clean
those up. DotNet and the JVM never GC their interned string pools,
right? If I were to clean it up, it would amount to writing a garbage
collection or reference counting system, so I'll revisit GC'ing
NameRefs when I have to look at GC'ing values.

# 2016-06-12-1516EST Type Merging is Incompatible with Capitalism

I've really been wanting to make the editing UI for loaded JSON files, and
it's very nearly there, except I realized I should probably finish one of the
fundamental operations in my type system: type merging.

The reason is because I want to load multiple JSON records into a
collection with disparate types, and then create a type for the
overall collection that I can use to help render the editor. So I went
through and thought out a bunch of the cases and wrote them all down:

A type merge takes two types, and creates a new type that both types
are compatible with.

`TYPE MERGE a, b` where at least one of `a` and `b` is a Union
should create a new Union whose cases are:

- For all Unions in `a`, `b`, include the cases of that Union in the
  result Union's cases

- For any non-Union in `a`, `b`, include that type in the result
  Union's cases

`TYPE MERGE a, b` where `a` and `b` are both not Compounds and both
not Unions should create a `Union(a, b)`. Array element types do not get
merged, i.e. `MERGE [Int], [String]` creates `Union([Int],
[String])`, and not `[Union(Int, String)]`.

The Array element type restriction seems like the right thing, but
might be too restrictive. Maybe it could do the restrictive merge if
both element types are primitive, since I know that at least *I* am
unlikely to want an array containing any of Int, String, or Compound.

Where it would make sense for me is for multiple Compound types in
order to support collections of supertypes. It could be a special case
that two Arrays whose element types are Compound result in an Array
with the two element types type-merged. It might also make sense to do
this for Union element types.

Whatever choice is made for default behavior, the end-user should be
able to redefine a collection's type so they can get the exact
behavior they want. A good initial UI for this might be to just pop up
a big text field with the type spec printed out in it, then allowing
them to edit that, type checking as they change it (continuous
feedback, like flycheck). I'll maintain a list of the set of unique
types for each top-level record in a collection, so that way this
feedback is given by just calling `check_type_compatible` for each of
the types in that set against the parsed type spec in the
text field.

`TYPE MERGE a, b` where exactly one of `a` and `b` is a Compound
should create a Union containing cases `a` and `b`

`TYPE MERGE a, b` where both `a` and `b` are both Compounds should
do a member merge

A member merge is partially defined by what `merge_compound_types` did
at the time this was written.

`merge_compound_types` will clone `typedesc_a`, and then for each
member of `typedesc_b`, will add it to the clone's members if names
and types do not match. Things get dicey here when members with the
same name have different types.

A new MEMBER MERGE operation will be defined to create unions when
member types do not match. An alternative would have been to just make
a Union of the two Compound types, but that would probably be annoying
in most cases, or at least for the top-level case. Maybe depth affects
which merging strategy is used.

`MEMBER MERGE a, b` clones `a` creating new type `m`.

For each member in `b` labeling it as `y`, try to find member in
`a` with a matching name labeling it as `x`

If a member `x` with name matching `y` is NOT found, then just add
`y` as a member to `m`.

If a member `x` with name matching `y` IS found and the types
match, then consider those members already merged.

If a member `x` with name matching `y` IS found and the types do
NOT match, then replace the type of member `x` with `TYPE MERGE
x.type_descriptor, y.type_descriptor`.

# 2016-06-05-2326EST How to render an editor

I don't really have a plan yet, but my last entry outlined some rough
pans and desirable properties that I want to try to explore further
and maybe even develop some concrete implementation ideas to
accomodate all that.

Last time, I wrote about wanting to switch between different ways of
rendering types. For Compound types with matching keys but mismatched
types for those keys, should I render different kinds of cell editors
for each row, or separate them? There are probably cases to be made
for both.

If you are viewing a Compound, or a Union with one or more Compound
cases, then you should be able to select a field in the Compound and
treat it as the new table type, "drilling own". Some records won't
have anything useful to render and edit here, so should they be marked
as UNDEFINED or just omitted? I assume there are good arguments for both.

The optional modes will be easy to do, but the drilling-down will be
interesting.

A simple thing would be to extract pointers to all the appropriate
records and put those in an list and walk through that list with a row
renderer.

That sounds annoying though, and that's really more of a cache. I
think the "proper" way is to have some sort of breadcrumb structure
that can point to what nested object in each structure you should be
rendering and editing.

To make things run faster, that breadcrumb can pull out the list of
pointers and treat it as a cache. Able to be recalculated at any time
should some new records be dynamically loaded.

Should I be allocating values in a general value store, so that I can
then sort a list of pointers however needed?

Or should I just assume I'll always point into a DynArray<Value>?

# 2016-06-02-0220EST The first row

So I finally made a little grid editor window for the first JSON file
loaded into memory.

There are a couple of problems:

## Text Input Length

First, `ImGui::TextInput` will use only exactly as many characters as there is
room for in your buffer.

Ideally you should be able to make a text field arbitrarily large, expanding as
you enter more characters, but there will have to be a practical limit. For
handling resizing, I thought of four approaches.

1. Whenver I run across a string field, expand its capacity to the maximum
   length. Pretty wasteful, especially since I'd like to support very large
   sizes if necessary (thousands of characters).
  
2. Ensure every string field has some extra room. This is like a more
   conservative version of the above. It would work, but is also kind of lame in
   that I'll do a bunch of allocations when I first display a bunch of
   records. Seems like the simplest to get going with.
   
3. Keep a separate edit buffer, and when a field is being actively edited, fill
   the edit buffer with that field's contents, writing back to the field if
   there is a change. This is cleaner, but the extra copying is annoying,
   considering that my Value struct is supposed to be all dynamic.
  
4. Ensure capacity like in 2, but only do it for the currently focused field
   like in 3. This sounds like The Right Thing. I need to make sure I grok
   dearimgui's focus/active semantics.
   
Since this is a project about "doing things right", I'm gonna try number 4.

## Heterogeneous Record Types

I'm sort of now confronted head-on with one of the problems I wanted to solve
with this tool, which is handling heterogeneous record types.

Here's me thinking through the problem:

I could merge all types into one type. I have functions that do this, and it
would work fine for cases where there the outermost type_id is always
Compound. The downside, is that now information will be lost, and the types
loosened.

Also, now that I think about it, I haven't thought of a clear way to represent
unions, so it feels like I haven't really solved any specific problems. What I'm
dealing with right now is actually just rendering an editor for a union.

The dumb way would be to divide it into N column sections, such that there may
be multiple columns with the same name, but in different Union members.

This seems kind of useless. I want flexibility.

I want to think about this more like the Keyset concept from Clojure's Spec
library, or at least what I understand to be keysets in that library. If a Union
has two cases which are objects, and their keys overlap, you should be able to
see that as one column.

BUT! What if there are different keys with differently mapped types in each of
the two Compound types? Should an individual cell be presented as sort of a
Union cell, where either one value or the other is shown? Or should that be a
separate cell? The right answer is probably be able to do both, because there
are probably use cases for both.

Another tricky thing is what to do when the top-level type is a Union consisting
of a combination of scalar, compound, and array type cases.

It's good to have some sort of systematic way of thinking about things, so here's some statements:

A loaded collection, a *table*, should probably have a single type. The way you
heterogeneous work is the collection is a Union type, and the individual records
have their respective types.

There will be rules for how to render a collection with TypeID of Union, and
cell / column with TypeID of Union.

You should be able to select a column and view it as a table, it's recursive
like that. You won't be able to focus in more if the TypeID of the table is
already a scalar. But, if it's a Union of scalar and compound, then you will be able
to focus in on one of the compound fields, or the scalar.

For rows which have no value for the current focus path, there should be a "no
value" representation (different from null, I suppose), with the option to hide
rows which have no meaningful value to show.

This is a lot of thinking. In the next dev log I will write down a concrete plan
of what to do next here.

## A wild libui appears

This library was posted on reddit a little while ago: https://github.com/andlabs/libui

I had seen this *andlabs* person on the golang irc channel talking about doing a
similar library for Go. Apparently he ended up splitting the platform-specific
UI interaction into this C library so that it coudl just be used in Go via cgo.

That makes me happy, because I'm concerned about making a polisehd versino of
this tool in dearimgui and will want to evaluate if libui might be a better
fit. I love this lib for prototyping right now, but people like their native
widgets, and I haven't really tried to push daerimgui to do anything
interesting.

On the other hand, @paniq seems to be getting a lot of milage out of dearimgui,
and I sort suspect that I'll have to do a bunch of custom rendering to make it
feel *really* good, so maybe there's no sense switching away from dearimgui.

# 2016-05-29-0048EST strings and Files and Directories Oh My

My next major goal is to load up a directory of JSON files so I can write a
litlte UI to edit their values.

But to do that I needed some more utility functionality, so I wrote DirLister,
which is a platform-specific OOP-lite interface to list all directory entries
under a given directory. I just did that macosx version for now, which uses
opendir/readdir.

It doesn't do a walk, which I thought I needed, but now I think I don't need.
This isn't there yet, but I can add an `access_path` field to `DirEntry` which
will just prepend the initial path parameter to the name of each directory
entry. I guess that's what `_ftsent.fts_accpath` is on bsd/linux. So, yeah,
maybe this is just reimplementing `fts.h`, but mine can be prettier and more
approachable, and provide less information. Which is a good thing, because I
really don't care about inode numbers, I promise. I also want a common interface
with a Windows implementation (via `FindFirstFile` and friends).

So I could walk directories using that thing, or if that hurts, I can always
make a `DirWalker` helper.

Errors have been annoying to deal with, until I realized that basically all my
platforms could usefully return platform-specific error code + message `Str`. So
I made a `PlatformError` struct that any sort of platform-specific operation can
return (mostly for filesystem crap). This is probably not the best long-term
thing (allocating for an error message seems a little excessive), but works
pretty well, and I could later change this to static error message pointers.

While making that I also added a few things to `str.h`:

- `str_make_copy`
- `str_copy`
- `str_overwrite`
- `str_append`
- `str_clear`

I had been using `FormatBuffer` for most cases where I wanted to concatenate a
bunch of strings together, rather than `str_concated`, since that one allocates
a new `Str`. Most languages I've used outside of C and C++ have immutable
strings, so the `str_concated` operation makes sense considering that sort of
flow, but this is not C# or Python or Elisp or whatever. While making
`DirLister` I found myself wanting ot just reuse a single `Str` object and
extend its capacity as needed as I hit longer and longer file names. So
`str_copy` is the workhorse, with `str_overwrite` and `str_append` being
implemented by passing appropriate parameters to `str_copy`.

While doing this, I've been more and more clarifying my usage patterns for when
to use `Str` versus `StrSlice`. My general idea:

- If you have a `Str` you should generally be aware of where it came from or
  what's responsible for it
- If you have a `StrSlice`, it's a view of a `Str` or a `const char *`, and in Rust-lifetime terms, should not outlive its source. 

Seems to have worked out pretty well. Something that felt uncomfortable at first
but proved to be a pretty good rule of thumb use point that if you have a
`StrSlice` to have its own lifetime, then allocate a `Str` for it. I think this felt
weird at first because I didn't have the `IAllocator` interface figured out.

Another thing that sort of clicked after reading a tweet(discussed a few
paragraphs ahead) from Casey Muratori is that you can't really null-terminate a
StrSlice whose source you don't control, so for cases where you absolutely must
have a null-terminated C string, you pretty much have to allocate a `Str`. This
is sort of obvious, and observable in my code, but having it clearly spelled out
was useful.

String storage is still not something I'm terribly happy with (`Str` always uses
`mem::default_allocator()` at the moment), but at least now I can see how many
string allocations I've made. Later I might want to add some sort of string heap
allocator. Heap allocators are annoying to implement, but I can probably do this one
in chunks and get away with it. It doesn't have to be general either, since I'm pretty
sure most other types in this program can use pool or stack allocators.

In the meantime though, the default allocator is fine. And a more immediate
memory problem that I would like to solve is getting persistent vector / bucket
/ deque style storage for `Type` objects. Right now, they use this `TypeRef`
thing which keeps an index and a pointer to the containing
`DynArray<TypeDescriptor>`, and it's a little annoying to deal with.

My thought for doing this was that I want to be able to dump the program state
to disk, and reload it with minimal pointer patching. This method probably
achieves that goal, but it might be easier to just do the work of translating
from pointers to indices and back for saving and loading states. I could also
try to do something like Shawn McGrath's virtual memory stacks. They're like the
Handmade Hero persistent storage, except the idea is to allocate as many as
needed throughout your program, and the addresses still work. There's some
details that make his implementation nice, but I'll have to watch one of his
archived streams (ugh) or wait until he releases the source code of his memory
tools to see exactly what he did.

Alright, break off from the program state / memory allocation tangent. One last
note about strings. There's one bit of shittiness where the `Str` is for
ownership and `StrSlice` is for views/references/borrows-for-rustaceans that
doesn't really fit my needs.

A couple of hashtables map `StrSlice` to a value, but in order to satisfy the
lifetime requirement, those `StrSlice` values actually "own" their char
data. It's irritating, but it's probably the only reasonable thing to do given
how the hashtables are implemented as templates.

Coincidentally, Casey Muratori posted this while I was working on this string
stuff:

https://twitter.com/cmuratori/status/736651441956257792

> I don't think I've ever encountered a common practical programming situation
> where I wanted null-termination as the convention for strings.

This is a great thread. I wish I could just grab all of these tweets and arrange
them into one mega tweet page, grabbing all of the Tweets that my out-of-date
Twitter for Mac app shows. I guess that's what Storify is for, but that looks
like a PITA to use.

The key takeaway was this tweet by Per Vognsen

https://twitter.com/pervognsen/status/736724036198137856

> The pointer + descriptor idiom is one of the best ideas in programming, which
> seems to be underappreciated and underused.

He then linked to a snippet of code from his Xeno foreign function/data
interface project, Xeno:

https://gist.github.com/pervognsen/d6ef883708465bd8f2b58640477a6e85

Tim Sweeney also chimed in, saying he wish the Unreal meta layer had followed
that sort of model and suggested forming this into a C++ introspection
proposal. 

https://twitter.com/timsweeneyepic/status/736752990690942981

Per elaborated on the difficulties of dealing with templates, and
points out Unreal's meta system with UClass (todo: look that up!) as an inferior
approach to plain-old-data descriptors. I'm guessing UClass implements some
virtual interface to provide metadata, as is typical of various C++ meta systems
I've seen people post on gamedev.net, /r/gamedev, et. al.

That whole twitter thread is great, and made me start thinking about the `Type`
and `Value` structures in this project. There's a similar thing going on in mine
and his code (inevitable for any sort of generic data manipulation system, I
guess), so I'm interested in seeing if there are any useful approaches I can
crib.

Something that I did see he does is sort of like Wren classes: fields in
aggregates are more about the offsets than they are about the names. The field
names are just their for the convenience of printing, it seems. I think for my
project, I'll want to emphasize field names

In a way it's slightly more like the recently introduced Clojure/spec library
(of particular interest is the key set concept): https://clojure.org/guides/spec

I think this is more in the right direction for this project, because I want to
be able to do transformations based on field name (sort of like my Unity
component clipboard).

Anyway, the last few remaining yak shaving things are a `clear` command for
clearing the console window (but not the list of log entries), and that
`access_path` thing for my `load_json_dir` function.

## 2016-04-27-0215EST Memory system lookin' kind of okay

After an embarassing memory corruption bug (and a journey to the land of linux
to seek out the AddressSanitizer to help vanquish said bug), I have got an
IAllocator interface and a Mallocator implementation for it that enforces
alignment and can associate metadata with it. For the moment, that metadata is
just the source line that `realloc` was called from, and an optional cateogry
string. Nothing is using that now, but it seemed like it would be useful.

I made the `AllocatorMetadata` a class, so that any `IAllocator` implementation
could use or ignore whatever metadata is available. There's also a
`FallbackAllocator` which just does straight `malloc` and `free` calls without
doing alignment or storing metadata or supporting `log_allocations`, so
naturally the `FallbackAllocator::realloc` implementation just ignores the
`meta` parameter.

I've started to use more OOP C++ features, at least for the memory system,
because I feel like the virtual allocator pattern is probably a good one. I also
watched Alexandrescu's talk about allocators to get some ideas, but it was more
about composing template parameters so get policy-based allocator template
classes, so interesting but not something I care about at the moment.

So what to do next? Probably those CLI commands to load a directory of JSON
files into a dynarray.

I also want to get a basic value editing UI in.

## 2016-04-18-0657EST Data sources and sadness

More data sources from Star Wars and Game of Thrones:

- https://tech.zalando.com/blog/your-favourite-franchises-are-having-an-open-source-love-affair-with-tech/

Sadness: I guess my `formatbuffer_v_writef` never really worked for resizing.

Now there are terrible bugs.

Testing fail.

## 2016-04-04-0114EST Test data sources

- https://github.com/toddmotto/public-apis
- https://github.com/joke2k/faker
- http://www.programmableweb.com/apis/directory

## 2016-03-31-0055EST Just make the app already

Okay, the main point of this program is to be like a spreadsheet for structured
data with the ability to create interesting views on the rows and do
sophisticated data validation and some type manipulation.

So now that I have value and type representations I need a collection of
records/values so that I can display and edit them.

I think that a good model for the final version might be that each collection
has a single type for all of its records. That type may be a union type, but a
single type feels clean.

Maybe that's unnecessarily applying nonsensical principles onto things, but it
feels nice to think about.

In the meantime, I can just render & edit each row based on the actual
value. Once I can load, edit, and save values, then I will go do a pass on
cleaning up memory management.

- [X] CLI command to load a directory of JSON files into a collection DynArray<Value>

- [X] CLI command to show an editing window for the value collection

- [] CLI command to save the DynArray<Value> back out to JSON files

For writing out JSON files, this looks like a nice library:

https://github.com/udp/json-builder

The author's parser might also be good to look into:

https://github.com/udp/json-parser

## 2016-03-20-1130EST Thoughts about naming types

So the type validator works, and was easier to write than I expected.

A big outstanding issue is being able to specify that members of a
Union/Array/Compound be of a named type, such that this named type can be
dynamically updated.

Say we define a position type:

```
type Position {
    x: Int
    y: Int
}
```

Let's now define an Icon type:

```
type Icon {
    pos: Position
    image: {
        cache_id: Int
        uri: String
    }
}
```

Now let's create this value:

```
OkButton = {
    pos: { 
        x: 42
        y, 23 
    }
    image: {
        cache_id: 0
        url: "ok_button.png"
    }
}
```

This would pass type checking against Icon.

But then if we edit Position to be like so:

```
type Position {
    x: Int
    y: Int
    z: Int
}
```

This should now fail type checking. The Icon type's internal representation
of its `pos` field should not have a direct TypeRef to Position, rather it should
be some sort of named reference that will do a dynamic lookup.

TypeRef should still be what it is, because fundamentally we need a stable way
to refer to types. There should be some other sort of type that can represent
either a direct type reference, or a bound type name.

Maybe TypeRef should actually be TypeHandle, and then TypeRef is a union of
TypeHandle and TypeNameRef or TypeSymbol or whatever.

Anyway, now we have an updated Position.

During normal execution of the eventual application, we'll need to figure out
what to do about this. You can't just say "all the data is now invalid!" because
that is stupid. In this case, we're just adding a field, and it should be
possible to create default values for any type. No data would be destroyed, only
added. So the editor should just do the correct thing.

Cases where you can change types and not have any data be destroyed:

- Add member to Compound

- Add type case to Union

- Convert a non-Union type into Union type containing the original
  type. E.g. change a `String` into `Union(String | Null)`, or `[Int]` into
  `[Union(Int | Bool)]`.
  
Cases where data *will* be destroyed:

- Remove member from Compound

- Change Array to Compound and vice versa

Cases where data *may* be destroyed:

- Remove type case from Union

- Change "leaf" type from one to another. E.g. Change `String` to `Float`.  A
  change such as `Int` to `Float` of course won't lose anything, and maybe it
  could even make sense to say that `Int` to `String` could do a reasonable
  conversion, but it would seem like something you'd want to offer as an option
  rather than as a default.

If a change to a type would cause data to be destroyed, then that calls for a
prompt to the user about how to proceed.

## 2016-03-19-0352EST Cleanup Todos

The code is gross. I mentioned before that I should make proper constructor
functions for various objects, but there's also basic renaming and such that I
want to do:

- [X] Rename TypeMember to CompoundTypeMember

- [X] Rename ValueMember to CompoundValueMember

- [X] Make a CompoundType to put in the TypeDescriptor union instead just the
     members array
     
- [X] Similarly, make a CompoundValue

- [X] TypeDescriptorRef should be TypeRef

- [X] Rename Value::typedesc_ref to typeref

- [X] Rename `typedesc_ref_identical` to `typeref_identical`

- [X] Add a nullptr define and make everything use it

- [X] Add Handmade-Hero style numeric type conversion functions with asserts in them.

## 2016-03-19: Concrete Union Plans

These are some concrete todos for Unions and type validation.

- [X] Create Union type representation and infer from JSON Arrays

- [X] Pretty Print Unions with `Union()` syntax

- [X] Write the `bindinfer` command to associate a type with a name.

- [X] Write a type validator and `checktype` command.

Something to note: Values should never have a type_id of `Union`. Unions only
exist in type descriptors. Values can pass type validation against a `Union`,
but there is no concept of a union value, the value's type is just one of the
union's type cases.

## 2016-03-17 Let's try Unions. Also, clean up value creation.

Unions are pretty rad. Let's try to make them work.

Right now I'm not explicitly making any types, just inferring them from JSON
strings, so the way I'm going to test this is using Arrays.

The goal is to have an Array be able to have multiple types (that you specify!)
in it.

There's another interesting thing that TypeScript and FlowType have called
    Intersection types, which sort of sounds like a multiple inheritance kind of
thing. That might be useful, but I'm going to pass on it for now.

After unions work, I want to be able to start validating values against
types. If I can't do that, there's no real point in making a type system thingy
at all.

To do that, I also need to bind types to names. I have a hashtable for that
sitting in `ProgramMemory` already, just need to add some CLI commands to infer a type
from JSON, and then associate a name with that type.

Let's say the command will be called `bindinfer` and you'd use it like this:

```
$ bindinfer "MaybeMysteryPerson" [{"firstname": "jim", "lastname": "joe"}, {"firstname": "jane", "lastname": null}]

MaybeMysteryPerson {
    firstname: String
    lastname: String | None
}
```

And now that you have this `MaybeMysteryPerson` type, you could pass other
values to it to see if they pass type checking:

```
$ checktype "MaybeMysteryPerson" [{"firstname": "jane": "lastname": "doe"}]
Passed

$ checktype "MaybeMysteryPerson" [{"firstname": "oh", "lastname": "henry"}, {"firstname": "noname"}]
Failed: Missing "lastname" field in $input[0]
```

The good error message won't be there initially, need to think about how best
track progress of walking a value.

Union types are cool because they enable you to do the whole No-Null thing,
which is probably a good thing. Nulls in JSON are annoying when they show up
unexpectedly, and annoying when you expect them to show up and they don't.

### Union Syntax

In the type description syntax for Unions, I'm thinking it might be useful to have
shorthand for union with None.

For example, take a person compound type where entering the last name is
optional, and so can be null, but first name must not be null:

```
Person: {
    firstname: String
    lastname: String | None
}
```

Typing out `| None` for a lot of things might get annooying, so maybe some
syntax sugar could add C#-style nullable type syntax, where this would be
transformed internally into the above when parsing:

```
Person {
    firstname: String
    lastname: String?
}
```

Only saved 6 characters, but hey, maybe you have a lot of nullable things.

That might be no good if I decide to make it lispy, such that type names can
have all sorts of weird symbols in their name, and tokens are separated by
spaces. That would mean you'd have to type it like this:

```
Person {
    firstname : String
    lastname : String ?
}
```

Not that bad really, I'm totally down. But people just love to crunch things
together for whatever reason.

I guess if you wanted the field to be named `"firstname:"` you could escape syntax symbols like
`firstname\:: String`. I dunno.

### Pretty Printing Sidetrack

Printing out Unions with compound objects will be tricky. Maybe something like this?

```
UnclearRotater {
    position: {
        x Float
        y Float
        z Float
    }
    rotation: |
        {
            x Float
            y Float
            z Float
        } |
        {
            x Float
            y Float
            z Float
            w Float
        } |
        Null
    scale: {
        x Float
        y Float
        z Float
    }
}
```

Or maybe just a function-like thing:

```
Union (
    {
        "a": String
    }
    [ Union (
        {
            b: Int
        }
        {
            c: Float
        }
        None
    ) ]
    type
)
```

That feels a little more readable.

## 2016-03-15 Array Success, Thinking abotu Unions

I started the array implementation last night, and now I've finished it.

There's a couple of gross things in there. First is what the json importer
should do if it detects a heterogeneous array. For now, it logs a warning and
produces an array of type None. This means there will be values floating around
with mismatched types. That sucks! Clearly I need to either reject this concept
entirely, or add a union concept.

Empty arrays suffer the same fate, just a different log message. Heterogeneous
arrays pretty clearly need some sort of Any type or Union support, but I don't
know what to do about empty arrays.

Compounds have a natural empty type. If you feed it a Compound with zero members,
then you get the empty Compound (or the typedescriptor is created if it hasn't
been seen before). An empty array doesn't make sense, unless we have some way to mark
it as having an invalid element type. But that sounds messy.

Maybe empty array signifies array of Any?

Maybe. I'm not sure. I'll just not deal with that for now.

I want to talk about Unions. This would kind of solve the Array problem, since
if it's a heterogeneous array, then that array's element type is the union of
the set of the types of its values.

So:

- Compounds have a list of members, where each member has a name and a type.

- Arrays have an element type.

- Unions have a set of types

There's a tricky thing here, which is that when assigning or validating
values, I'll have a value of a non-Union type, and will want it to be assigned to
or pass validation for a Union type location.

I could think about this as just dealing with concept of a value type versus a validation type.
Or, it could be that non-Union Values get converted to Union values with a selected type tag.

That second version feels more like a C union, so maybe that's the right thing. I should probably
look at the implementation of Unions in some statically and gradually typed languages.

- Typescript
- MagPie / Finch
- Dart (does Dart even have Unions?)
- Haxe
- *maybe* Rust
- Ceylon? Kotlin? ...Scala? *shudder*
- Typed Clojure
- Typed Racket
- Do SkookumScript or AngelScript or Squirrel or GameMonkey have types?
- Perl 6 / Rakudo

### Side Note: dynarray why

Apparently there's a `std::experimental::dynarray` for fixed-size heap allocated
arrays. This kinda conflicts with my `DynArray<T>` and `dynarray_init`
names. Don't know if I care enough to do anything about it.

## 2016-03-14 Expand the Type System

Now I have a graphical CLI working relatively well. I can easily add commands to
process values I enter at the command line with json syntax.

Now I need to start building out the rest of the type system.

Notably missing are:

- Arrays
- Enumerations
- *Maybe* Unions
- *Maybe* an Any type

Arrays and Enums are essential because I use them all the time in C#, and all
languages can pretty much represent them in a reasonable way.

Unions are tricky since they don't really exist in C# and other languages (at
least not without doing tricky things with GCHandles and structs and special
layout attributes), but they sure are nice.

Unions are also useful because ultimately you'll need to be able to specify the
type of an Array, and if you really want an Array to have maybe one thing and
maybe another thing, then that's a Union!

If an Any type is supported (fairly likely, I mean the main focus here initially
is editing JSON files in a directory) then you can have an Array of Any.

I don't want to start with Any though.

I don't want to start with Enums either, because I need to ruminate on them a
bit more. The ruminating involves questions about representation, width, value
names, and Tuples. Yes, Tuples! I thought about them while thinking about
how to represent the type of an Array.

But Arrays I think I have a handle on. Arrays should have a single type
parameter. I noted above that heterogeneous Arrays could be accomodated via an
Any type, so that frees me up to say that Arrays are just of a single type.
It's also how statically typed programming languages usually work so I'm just
gonna do the obvious thing.

### Unrelated Note about value storage

Right now a ValueMember's Value object is stored inline, and based on my
experiences with my Mal interpreter, those should probably be pointers or
handles.

This type system definitely has Javascript/Java/Python-like pseudo-reference
semantics. This means I can have a big Value memory pool, opens up the
possibility of having a reference type, and probably makes value initialization
and copying (e.g. from one Array to another) much cleaner.

Could also maybe open up clever optimizations for many duplicate values, e.g.  a
CoW kinda thing. That sounds tricky though, gonna look at that further down the
road.

## Refactor this README's TODOs

I'm going to maintain a list of todos at the top, and only list todos in the
individual dev log entries with a "- []" prefix to indicate their todo-y-ness,
but will then copy them up to the top with a log entry prefix.

I'm also going to start adding the day to each log entry for disambiguation and
yay record keeping.

## Milestone: Solid GUI Terminal

The dearimgui console is functioning. Now I want the command execution stuff to work
with Value arguments again (now with objects yay!), and I want all the interactions
to feel good and do useful things.

- [X] Get `exec_command` working again

- [X] Decompose all the `run_terminal` functions into parsing / exec'ing
      subroutines. They're all pretty redundant right now

- [X] Up-arrow history

- [X] Auto-scroll to bottom on output

- [X] Auto-focus the input field the first time

- [X] Selectable output for copy-paste

- [] Test/develop on Windows

## Memory

I've been a little sloppy. The fact that I pass around value objects means that
it's not cut-and-dried when parts of a Value or TypeDescriptor are done being
used. This gave me problems when trying to implement GC in my Mal implementation,
so I should maybe have my own object heap.

In the meantime, after the dearimgui console is done:

- [X] Free the memory used by the CLI value args

- [X] Wrap malloc and free in order to count allocations

## DearImgui works, now what?

I may have been a little overzealous in making my tokenizer routines. It will
probably be useful when I need to go and make special syntax for defining types,
but for parsing values at the CLI prompt, I probably should have just parsed
json.

Reason I say this, is because although I can do numbers and strings well enough
in the tokenizer, now I want to be able to test with objects, and that's not a
thing that I can just immediately do super easily.

Now that I've got the dearimgui demo running, I want to run the CLI as a console
window in the GUI.

- [X] Use json.h instead of home-brewed parser for CLI argument parsing

- [X] Make sure defining objects on the CLI works.

- [X] Have a single console window in the GUI window and run the CLI in it.

## Freeing Memory of Hashtable Keys

Something kinda clicked in my brain today. I always found the iterator-heavy C++
container APIs to be annoying to use, but today I discovered a reason for them
to be the way they are.

I don't have a strong memory management strategy yet, but I'm at least not
introducing any (or at least few) memoy leaks. I was about to add one when
registering commands by mapping StrSlice to Command structure. I didn't yet feel
comfortable making Str a key of a hashtable, so to store values I use a
OAHashtable<StrSlice, Value>, where the convention is that those StrSlice values
own their memory.

This totally breaks my convention, but I want to be able to use StrSlice to do a
lookup into this table, so I wanted to make the types match, it's just easier
that way.

(I've been on purposing not using any C++ features besides templates and namespaces,
and whie this tempted me to think about using destructors, I don't think it's necessary.)

Anyway, that was a tangent!

The reason why I think it's good for my hashtable to support the ability to
return an iterator / entry object is so that I can replace the key after adding
or finding. Sounds horrible, right? Well, it could be if you were trying to
program super-defensively, and I might try to get rid of thisin the future, but
basically I need to be able to allocate dedicated memory for the key without
having to go specialize that template for particular type or add a type parameter
or something for an allocator function.

This might bite me, or it might be totally fine. I don't have the experience to
tell.

## What now?

I now am able to parse a JSON file, infer some types, build a structure, and
build a table of type descriptors such that there are no shared type structures.

The code that ensures there are no duplicate equivalently-structured types
descriptors is a little dumb (`find_equiv_typedescriptor`), but it works, and
can be turned into a hash lookup later.

Things that this system is missing are: arrays, enums, and unions.

I think I can move forward without implementing those just yet, because I want
to work with the TypeDescriptor code a little more before making any decisions
that might have to last a while.

Things that I could tackle next:

- Basic UI, integrate dearimgui and start drawing a table of editable values

- Even more basic UI, just integrate dearimgui enough to make a command prompt

- Terminal command prompt, just some basic command processing with type
  inference (strings, integers, floats)

- Representation of Values as bound to an associated type

- Named Type Ref: A type member should be able to refer to a typedescriptor by
  name, and update when that type updates
  
I think the most critical missing piece is a value representation. After all,
what good is this program if it doesn't know about value? However, I also
think I need to do a basic command line parser, because when do start dealing with
values I want a quick way to set/unset/change type to verify things are working.

So next steps:

- [X] Command reader that works like the following:

  First token is a symbol/string that must refer to a registered command

  Each subsequent token is parsed into one of the following types:
  
  - String: lookslikethis "looks like this"
  - Int: 123432 -12309 +1238923
  - Float: 123.321 -123.321 +123.321
  - Bool: true false
  
  No compound literals(yet).
  
  Commands will be things like setvalue, and will probably rely on creating
  named values, and operating on the value named by the first string parameter

- [X] Represent and store values, and be able to change them via CLI.

The parser could become a really integral part of this. It would be cool if the
syntax for type definition files and the syntax for editing values in cells and
for inputting into the command line were all very similar.

## Expanding on type refs, bindings, and descriptors

So what's the situation with the "symbol table." It's really the binding
table. Let's call it that.

The idea is to associate names with TypeDecsriptors. Why do I care about this?

The advantage of having type names is that you can change the definition of the
type, and anything referring to that type by name will be converted to the new
type representation.

So TypeMembers should actually have a TypeBinding, which has a TypeRef, refers
to a TypeDescriptor.

Why do TypeRefs exist? Well, we want "anonymous" types, because that's a useful
concept, and you might want to import a bunch of JSON files, design a type
schema to assign to them, and then set the type for a row, column (or cell??) to
be that named type. And, if you then want to make a change to all rows that now
have that type, it should be possible to propagate that change every place where
the type is referenced.

## Important Feature

Data views.

Every time I want to edit the PowerUpMods for any of the vehicle parts, I have to:

1. Freeze the window to lock the ID column and header rows
2. Hide all columns except the ID and Special Effect columns
 
It's irritating. It should remember my previous view and just open to that, or
have that configuration quickly accessible.

## Next steps

- [x] Make a Type Table that you can search by type structure. This means that there will
  probably be a bunch of temporary TypeDescriptors being created when interpreting types
  from JSON. This is okay, we can use a scratch allocator for those.

- [x] Make a symbol table that contains bindings from names (symbols, so they're
  interned) to stored TypeDescriptors.
  PROGRESS NOTE: Made a hashtable! This can be the lookup structure for both symbols and bindings.
  PROGRESS NOTE: Made a name table! I wanted to store and intern all names, and this accomodates that.
                 Now a symbol table can map NameRefs to TypeDecsriptors
  
- [x] Get this in a git repo.

## Modifying types, and propagating those chagnes

So the previous entry talked about how Type Descriptors should be immutable.

That definitely cleans stuff up (and is sort of a requirement for the hashtable).

But I also want to be able to edit types: add/remove members, add/remove
validations, change member types, or rename the type.

Getting this right will be essential, because renames and structure changes are
almost always a brittle process, and part of the purpose of this tool is to
alleviate that by exposing fundamental transformations in the UI that are
familiar to people who work with structured data in the form of JSON, XML, or
Unity serialization.

Possible ways to make changes:

- Change the TypeDescriptor directly. This is more of a strawman. Since type descriptors
  are shared, this will cause breakage.

- Collect all uses of TypeDescriptor, set those to a new TypeDescriptor, then
  edit the new TypeDescriptor. This has similar problems above: how do you
  choose which other types want their sub-field types to be changed, or which
  values should be changed?
  
- Rebind names. This makes a lot of sense, except for the case of member types that
  don't have a name binding.

- Maybe that's the lower level mechanism: if a type has a name binding, then you can
  make edits to the type that propagate throughout all the other types.

Making presence of a name binding is interesting, because it kind of subverts the usual
way of thinking about types, which is them being a flat record.

You could have a `Doodad` type defined like this:

```
Vector3f {
  x: Float
  y: Float
  z: Float
}

Doodad {
  name: String
  position: Vector3f
}
```

Or like this:

```
Gizmo {
  name: String
  position: {
    x: Float
    y: Float
    z: Float
  }
}
```

The `Doodad` type could have the definition of Vector3f changed out from under
it, and that should be handled just fine, whereas `Gizmo` is not thought of as
having a type within itself, rather the nested compound types are part of
`Gizmo`, an analogy being value versus reference semantics.

So a TypeMember now doesn't just have a pointer to a type_desc, it actually has
something like a union of TypeDescriptor OR a TypeDescriptor name binding. Or
maybe handles are just bindings, but there's a special rule that there can be
multiple "anonymous" TypeDescriptor bindings.

Or better yet, maybe we do the compiler-style thing where we generate hidden
names for anonymous TypeDescriptors. They will still need a flag indicating
their anonymity, and maybe that means that this idea isn't that good, but at
least it would probably everything to work in the same flow.

## New thoughts on storage and equality and immutability

I've been wondering about how to deal with mutating TypeDescriptors, and I now
think that it's pretty clear that they should be mostly immutable once you're
done construting them (setting `type_id`, calling `add_member`, etc.).

This way I can have a type registry that is a hashtable of the type descriptor's
contents, as well as a table of names bound to types.

This would reduce the cost of checking for equality in type descriptors, since
instead of having to recursively do an equality check on every member's type
descriptor, you could just do a pointer compare.

It might be necessary to use handles/slotmaps instead of pointers since this is
starting to look like Casey's entity storage mechanism from Handmade
Hero. Probably want to add some sort of bake/finalize operation for type
descriptors.

When values are read, they are given TypeDescriptors, but not necessarily very
strict type descriptors. I think it makes more sense to give each an "evaluated"
type descriptor, like when making TypeDescriptors from JSON, which is again a
ref to a stored TypeDescriptor. I'll probably want to create and set some global
refs to the primitive types at startup.

[[ NOTE: When I say global refs, I mean set in a sort of ProgramMemory sturct. This
stuff should be trivial to dump to and slurp from disk (all the more reason to
eventually use handles instead of pointers). ]]

Anyway, back to values. Values each have a ref to a TypeDescriptor that matches
their raw interpreted type. There's an implicit notion that values are read as
either a compound type or one of a few primitive types. I want to be able to
convert between named types if the structure matches appropriately, or be able
to take one member column out of a record and put it into another record's
column if the TypeDescriptors are equal, or could be converted easily by just
translating some member names.

In terms of interpreting fundamental types, I wonder if there's a standard way
for integer types to be inferred other than looking for floating point numbers
without a fractional component. If it could work sort of like regular
programming language syntaxes where no decimal point means integer, then that
might be useful. I would need to modify sheredom/json.h to make that happen.

There are other ways that type info could be loaded in, though. Something like
JSON-Schema would probably be useful for people. Maybe not that specific format,
but whatever is out there and is popular with some preexisting tools.

Another good avenue would be a Unity3D script that can generate type descriptor
definition files from System.Type objects. That means that there should be a
standard text format for type descriptors. I think it would be best if the
syntax ressembled Jai/Go/Rust/AS3 slightly, with post-fix typenames (kinda like
how the pretty_print output looks now).

Or maybe it should match that Open Game Data spec that Eric Lengyel amde.

## Initial TODO

- [X] Merge type descriptors

  This feels tricky on the surface because I'm doing it from two arbitrary JSON
  files, but the important idea is that if the field names match, then the record
  matches. If you see a `gold_value: float` member in type descriptor A and
  `gold_value: float` in type descriptor B, then those are on and the same member.

  One tricky case is what to do about type mismatches. Some possibilities are:

  - Reject the two type members as unmergeable (that sounds like it sucks)

  - Have the first type member to override the second (that sucks less, but
    still sucks, because you lose data)

  - Allow the two type members to coexist, and resolve in one of these ways:

    - write out whichever field exists on a particular record, show both in the
      UI (this sounds dumb but clean and simple)

    - require the user to resolve them before saving (sounds tedious, would be
      good to have this as an option, but not a requirement)

    - make a fancy sum type concept (this sounds hard, but might be cleaner, or
      might be more annoying UI-wise)

  I feel like I can at least say that At a minimum, one TypeMember's equivalence
  to another is formed by its name and its own type descriptor. Its type
  descriptor may or may not need a name, that feels like a slightly broken part
  of the data model right now.
  
  Breaking news: maybe sum type is the right way to go. The reason is that, what
  happens when you try to merge two non-compound types? Or one compound type
  with a non-compound type? The answer is you can't without a kind of sum type.

  Unfortunately that presents UI challenges. What does a non-compound type look
  like when displayed as a row? What does a union of compound and non-compound
  types look like?
  
  Maybe I could support union types in the lower layer of the infrastructure,
  but prevent that in the UI/API layer?

- [X] Load values in

- [X] Validate values against type descriptors.

- [X] UI. Was looking at Qt. Maybe that won't suck. it probably will. dear imgui?
  *NOTE: decided to start with dearimgui

  Let me list the actual possibilities:

  - Qt
  - WxWidgets
  - GTK (GTKmm?)
  - dearimgui
  - OUI/blendish
  - GWEN? https://github.com/garrynewman/GWEN Seems old
  - OtterUI https://github.com/Twolewis/OtterUI
  - TurboBadger https://github.com/fruxo/turbobadger
  - MyGUI http://mygui.info/
  - Game-GUI https://github.com/sp4cerat/Game-GUI
  - NVWidgets https://code.google.com/archive/p/nvidia-widgets/source
  - LibAgar http://libagar.org/
  - Chromium Embedded Framework https://bitbucket.org/chromiumembedded/cef
  - Awesomium http://www.awesomium.com/
  
  Interesting Links:

    - http://www.gamedev.net/topic/574795-designing-a-gui-system-hints-appreciated/
    - http://osdir.com/ml/games.devel.sweng/2005-09/msg00042.html

  The UI should be SUPER SNAPPY. No jitteriness due to widget update slowness or
  awkward APIs that make me less likely to implement something good.

  It will be difficult to get a perfect per-platform juiciness because I am not
  able to write a per-platform UI. I just refuse. What I do care about is text
  field editing conventions per-platform, such as Cmd-Left/Right versus
  Control-Left/Right on OSX and Windows behaving correctly.

  Beyond that, there's not much concern. Tab, return, etc. will probably all
  function the same way. Mouse will function the same way.
  
## Important Aspects

Saving out a single row/record/json-file is ESSENTIAl.

You want instant feedback on the validity of a row, so you can get in, make a
change, get out. Or incrementally make changes.

If there's some sort of notification mechanism to tell Unity or Unreal or
Lumberyard or whatever engine that a change has been made, then that integratino
should be available and delightful to use.

## General Project Notes

A project has to be solving problems, and generally the problem here is that editing
game data in spreadsheets sucks


The goal of this thing is to be able to edit a directory of JSON files as though
you were working with a spreadsheet.

It should feel like Google Sheets, but do validation like a Unity inspector
does, but also do a sort of "gradual typing" while editing, such that it can
dynamically generate mising members for compound type descriptors, and if you
change the schema, it should not immediately drop a bunch of data. It will
eventually have to drop data once the user quits out, but until then, data
should be recoverable.
                                                                                                     
My use case is to reflect on a bunch of C# types, generate a schema file for
this tool, then work with the json files in this tool instead of exporting from
a spreadsheet/TSV. Records (json files) are presented as rows, and you should be
able to reorder rows in the view, as well as sort and filter all of the loaded
records. You should also be able to hide columns, or delete columns entirely
from the schema and therefore from all of the loaded records.

It should handle nested records. E.g. I want to be able to represent
something like this:

```
{
    "rotation": {
        "x": 0.234,
        "y": 0.44,
        "z": 0.1,
        "w": 1.0,
    },
    position: {
        "x": 0.243,
        "y": 0.55,
        "z": 0.1,
    }
}
```

You should be able to see multiple views of this:

ex:

```
fileid    | rotation   | poisition
----------|------------|-----------
somefile0 | [multiple] | [multiple]
somefile1 | [multiple] | [multiple]
somefile2 | [multiple] | [multiple]
somefile3 | [multiple] | [multiple]


fileid    | rotation.x | rotation.y | rotation.z | rotation.w | postion.x | position.y | position.z
----------|------------|------------|------------|------------|-----------|------------|-----------
somefile0 | 1234.5678  | 1234.5678  | 1234.5678  | 1234.5678  | 1234.5678 | 1234.5678  | 1234.5678 
somefile1 | 1234.5678  | 1234.5678  | 1234.5678  | 1234.5678  | 1234.5678 | 1234.5678  | 1234.5678 
somefile2 | 1234.5678  | 1234.5678  | 1234.5678  | 1234.5678  | 1234.5678 | 1234.5678  | 1234.5678 
somefile3 | 1234.5678  | 1234.5678  | 1234.5678  | 1234.5678  | 1234.5678 | 1234.5678  | 1234.5678 

fileid    | rotation.x | rotation.$rest | postion.$rest | position.y
----------|------------|----------------|---------------|-----------
somefile0 | 1234.5678  | [multiple]     | [multiple]    | 1234.5678 
somefile1 | 1234.5678  | [multiple]     | [multiple]    | 1234.5678 
somefile2 | 1234.5678  | [multiple]     | [multiple]    | 1234.5678 
somefile3 | 1234.5678  | [multiple]     | [multiple]    | 1234.5678 
```

The $rest notation is an idea, maybe not that useful. But the idea is that those
[multiple] cells would actually allow you to show an inline tree editor.

I have noooooooo idea how to deal with arrays yet. Maybe they should just be
viewable in another table editor, treating them kind of like some sort of
relation as if in a DB.

I also want to support structural manipulation. Kinda like my component
clipboard that I made for Unity, but way more malleable.

I think that a good working assumption is that you'll be working in a sort of
MDI/tabbed interace, and that each open document will have its own
type-descriptor for the records loaded.

So if you want a sort of scratchpad, you can duplicate your current tab, or
"copy" a bunch of records from your current tab and load them into a new tab
(maybe a command like "new tab from records").

This will clone the type-descriptor hierarchy and copy the selected records
over. A later optimization could do a Clojure/persistent-data-structures-style
copy-on-write thing.


some constructed test data defs:

```
Vector3f
{
    x: f32
    y: f32
    z: f32
}


RPGItem
{
    display_name: String
    gold_value: Int
    drop_probability: Float // [0, 1]
    min_level: Int
}
```

some real test data defs:

```
GameItemDef
{
    // easy
    string id; // join point
    string display_name;
    string flavor_text;
    string garage_visual;
    string ingame_visual;

    // not as easy
    InventoryItemType type; // enumerated type: string or int?
    ItemRarity rarity; // enumerated type: string or int?

    // hard
    string[] found_in_boxes; // todo: later
    GameItemFlags flags; // ooh flags how do
}


SpecialEffect
{
    SpecialEffectTarget target; // enum string
    SpecialEffectProperty property; // enum string
    float numValue;
}


VehiclePartDef
{
    string id; // join point

    VehicleSlotType[] slots = new VehicleSlotType[0]; // enumerated type: string or int?
    string setWheels;

    VehicleAttributeModifier[] // compound types
    SpecialEffect[] specialEffects; compound types
    PowerUpTypeID[] addPowerUps; // enumerated type: string or int?
    PowerUpTypeID[] removePowerUps; // enumerated type: string or int?
    int adjustWeight;
}
```
