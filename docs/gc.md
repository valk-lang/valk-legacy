
# GC

## What the GC does

```
# Local variables are stored on the stack:
> stack item structure: 4x ptr per item -> { item_adr, item_vtable, prev_adr, prev_vtable }

# All new items have start with state: 0

# Changing a object property, we call: linking
> If we link an item to another item with state > 2
> We add that item to the transfer list (state: 2)

# When the pool reaches a certain size ~80%
# or reaches the lowest stack point with size ~10% or more
# We call gc:
> Step 1. Loop over the stack
- Any item with state 0 will be added to the transfer list
- Any previous item that no longer on the stack is added to the unknown list (This step might be skipped, see step 5)
> Step 2. Handle transfer list
- if state < 4 : set state = 4 (recursive)
> Step 3. Loop over all pool items
- if state is 0 : call item._gc_free() using it's vtable
> Step 4. Reset the pool
- set pool current = pool start block
> Step 5. Check if we need to do a full stack trace clear
- if transfered-bytes > last-marked-bytes
-- marked_bytes = 0, transfered_bytes = 0
-- mark stack with age (recursive)
> 5.1 : Option 1
-- loop over all pool blocks and their items
--- if state > 0 && age != gc_age : set state 0 && call item._gc_free()
> 5.2 : Option 2
-- loop over all unknown items (recursive)
--- if state > 0 && age != gc_age : set state 0 && call item._gc_free()
```
