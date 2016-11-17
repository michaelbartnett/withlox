# Withlox

(see the section towards the bottom about code style if the dearth of constructors and C++11 is making you uneasy)

## What Is It

This is a work-in-progress tool that loads a directory of files, treating each as a row in a table and presents them in a spreadsheet-esque editing UI.

CastleDB is kind of related, but it's also sort of trying to be a level editor, and its type system is limited, and the UX work seems to be going into the mini scene editor part of it rather than the table editor interface:

http://www.gamefromscratch.com/post/2015/11/14/CastleDB-A-Game-Database-A-Level-Editor-Its-Both.aspx

## Why Are You Writing It

Primarily to scratch an itch, to keep my C and C++ skills sharp.

The itch being scratched is best explained here:

https://twitter.com/michaelbartnett/status/707762227907010561

> Mike Acton @ mike_acton 9 Mar 2016
> \#gamedev Designers: What's one tool you really wish you had right now? Be as specific as you can! 2014: http://bit.ly/1P9oMqz
>
> --------------------------------
> Michael Bartnett @ michaelbartnett 9 Mar 2016
> @ mike_acton I'm a programmer, but something we want and don't have time to make is a generic structured data editor that feels like a
>
> --------------------------------
> Michael Bartnett @ michaelbartnett 9 Mar 2016
> @ mike_acton spreadsheet but does smart things w/ game's data types. The flexibility & UX of google sheets is useful. Esp. formatting options
>
> --------------------------------
> Michael Bartnett @ michaelbartnett 9 Mar 2016
> @ mike_acton and filtering and sorting & explicitly reordering rows w/ click-n-drag. LibreOffice and TSVs good enough, could be much better.
>
> --------------------------------
> Jurie Horneman @ jurieongames 10 Mar 2016
@ michaelbartnett @ mike_acton Better generic tools for graphs and non-Excel grids. I can do magic with Excel if the data fits.
>
> --------------------------------
> Jurie Horneman @ jurieongames 10 Mar 2016
> @ michaelbartnett @ mike_acton But if the data doesn’t fit Excel, it gets harder.

## When Will It Be Done

This is my "slow burn" side project.

## Supported Platforms

Mostly developing on OSX while I get things working and experiment with what's useful. I also periodically test on Windows 7.

## UI

ocornut/dearimgui for now :)

That won't be long-term, but it's been a very nice starting point while I get the type system and other bits in place.

## Code Style

I'm writing this in a style somewhat influenced by Handmade Hero and Bitsquid. This means a strong preference for free functions over member functions, template usage restricted to containers for the most part, and many of the types related to application logic are trivially constructible, the containers assume trivial types, and thar be macros. Compiles with `-Wpedantic -std=C++03`.

It's unclear to me still if I like doing things this way or not, but I wanted to give it a go. I'm considering leaving the dark ages soon and switching to C++11 so it's easier to enforce the container restrictions with type traits. Having `auto` and using some captureless lambdas would also be nice.

This is my "see how different things feel" project. I'm even doing newlines for all braces instead of K&R style. Inconceivable!

## What's with the name?

Well, it used to just be called "jsoneditor", but now that I'm putting it on github that's way too literal. It's supposed to vaguely feel like spreadsheet, so word assocation time: spreadsheet...spread...schmear...cream cheese, bagel, lox. I'm kinda feelin' a bagel right now.

I'll figure out a better name later.

## Well, Anyway

Thanks for checking out my project! There's a ways to go yet, but suggestions/criticism welcome.
