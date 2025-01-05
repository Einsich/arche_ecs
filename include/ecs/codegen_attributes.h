#pragma once

// you can add attributes inside of the macros
// for example:
// ECS_SYSTEM(attribute1 = value1, value2; attribute2 = value3)
// system_name(type1 name1, type2 name2)
// {
// ...
// }

// attributes:
// require - list of components that system should have, but can not query them
// require_not - list of components that system should not have
#define ECS_SYSTEM(...) static void

// attributes:
// require - list of components that system should have, but can not query them
// require_not - list of components that system should not have
#define ECS_QUERY(...)

// attributes:
// on_event - list of events that system will be subscribed to
#define ECS_EVENT(...) static void

#define ECS_REQUEST(...) static void
