extern(C) void call_closure(void* obj, void delegate()* func) {
	func(obj);
}
