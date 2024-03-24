
# Parser

## Build flow

```
# A build command creates a Build instance
# The Build instance contains:
- A parser object (in case we switch to multi threading: 1 parser per thread)
-- A parser keeps track of what and where we are parsing
- ~6 stages
# We look at our cli arguments and add the .va files to stage-1 of the build
# We can now start building by looping every stage and passing the file to a parser
# If a parser is done with a certain stage, it passes the file to the next stage in the build 
# Once all files have gone through all stages, we now have all our IR files
# To finish the build we convert our IR files to object files and link them together
```

## Parse flow (Stages)

```
# When adding a new file (Fc) to our build we load the file content
> and send it to the lexer which returns a Chunk containing our tokens
# Stage 1
- Pass fc.chunk to parser.chunk
- Loop tokens to find all outer definitions: class, func, global, ...
# Stage 2
- Read class properties and functions
# Stage 3
- Read all types: class properties / function arguments / global types ...
# Stage 4
- Parse AST
- Generate IR
```
