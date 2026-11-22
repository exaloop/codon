# Copyright (C) 2022-2026 Exaloop Inc. <https://exaloop.io>
"""ORC object allocation, GC roots, and debug objects for the C linking layer."""

from collections import deque
from ctypes import CDLL, CFUNCTYPE, addressof, c_char, get_errno, string_at
from dataclasses import dataclass, field
import mmap
import os

from .llvm import A, I, N, P, S, U

LAYER_CREATOR = CFUNCTYPE(P, P, P, S)
CREATE_CONTEXT = CFUNCTYPE(P, P)
NOTIFY_TERMINATING = CFUNCTYPE(None, P)
ALLOCATE_CODE = CFUNCTYPE(P, P, N, U, U, S)
ALLOCATE_DATA = CFUNCTYPE(P, P, N, U, U, S, I)
FINALIZE_MEMORY = CFUNCTYPE(I, P, A)
DESTROY = CFUNCTYPE(None, P)
OBJECT_TRANSFORM = CFUNCTYPE(P, P, A)


@dataclass
class Segment:
    memory: mmap.mmap
    base: int
    size: int
    protection: int
    rooted: bool = False


@dataclass
class JITObject:
    data: bytes | None
    segments: list[Segment] = field(default_factory=list)
    text: list[tuple[int, int]] = field(default_factory=list)


class JITMemoryManager:
    """The writable-segment registration and lifetime of the C++ memory manager.

    LLVM-C exposes allocation callbacks on RTDyld. The C++ JITLink memory
    manager receives the entire layout up front, while these callbacks receive
    individual sections, including linker-created sections.
    """

    def __init__(self, library, debug):
        self.lib = library
        self.debug = debug
        self.pending = deque()
        self.objects = {}
        self.next_context = 1
        self.add_roots = library.runtime_function("seq_gc_add_roots", None, P, P)
        self.remove_roots = library.runtime_function("seq_gc_remove_roots", None, P, P)
        self.mprotect = CDLL(None, use_errno=True).mprotect
        self.mprotect.argtypes = (P, N, I)
        self.mprotect.restype = I

        # LLVM retains the callbacks until it destroys the object layer.
        self.create_layer = LAYER_CREATOR(self._create_layer)
        self.create_context = CREATE_CONTEXT(self._create_context)
        self.notify_terminating = NOTIFY_TERMINATING(self._notify_terminating)
        self.allocate_code = ALLOCATE_CODE(self._allocate_code)
        self.allocate_data = ALLOCATE_DATA(self._allocate_data)
        self.finalize_memory = FINALIZE_MEMORY(self._finalize_memory)
        self.destroy = DESTROY(self._destroy)
        self.capture_object = OBJECT_TRANSFORM(self._capture_object)

    def _create_layer(self, _context, session, _triple):
        layer = self.lib.function(
            "OrcCreateRTDyldObjectLinkingLayerWithMCJITMemoryManagerLikeCallbacks",
            P, P, P, CREATE_CONTEXT, NOTIFY_TERMINATING, ALLOCATE_CODE,
            ALLOCATE_DATA, FINALIZE_MEMORY, DESTROY,
        )(
            session, None, self.create_context, self.notify_terminating, self.allocate_code,
            self.allocate_data, self.finalize_memory, self.destroy,
        )
        if self.debug:
            listener = self.lib.function("CreateGDBRegistrationListener", P)()
            if listener:
                self.lib.function("OrcRTDyldObjectLinkingLayerRegisterJITEventListener", None, P, P)(
                    layer, listener
                )
        return layer

    def _capture_object(self, _context, object_in):
        try:
            obj = object_in[0]
            size = self.lib.function("GetBufferSize", N, P)(obj)
            address = self.lib.function("GetBufferStart", P, P)(obj)
            self.pending.append(string_at(address, size))
            return None
        except (MemoryError, OSError) as error:
            self.lib.function("DisposeMemoryBuffer", None, P)(object_in[0])
            object_in[0] = None
            return self.lib.function("CreateStringError", P, S)(str(error).encode())

    def _create_context(self, _context):
        context = self.next_context
        self.next_context += 1
        self.objects[context] = JITObject(self.pending.popleft() if self.pending else None)
        return context

    def _allocate_code(self, context, size, alignment, _section_id, _name):
        return self._allocate(context, size, alignment, mmap.PROT_READ | mmap.PROT_EXEC)

    def _allocate_data(self, context, size, alignment, _section_id, _name, read_only):
        return self._allocate(
            context, size, alignment, mmap.PROT_READ if read_only else mmap.PROT_READ | mmap.PROT_WRITE
        )

    def _allocate(self, context, size, alignment, protection):
        try:
            alignment = max(alignment, 1)
            length = max(size, 1) + alignment - 1
            length = (length + mmap.PAGESIZE - 1) // mmap.PAGESIZE * mmap.PAGESIZE
            memory = mmap.mmap(
                -1, length, flags=mmap.MAP_PRIVATE | mmap.MAP_ANONYMOUS,
                prot=mmap.PROT_READ | mmap.PROT_WRITE,
            )
            base = addressof(c_char.from_buffer(memory))
            address = (base + alignment - 1) // alignment * alignment
            obj = self.objects[context]
            segment = Segment(memory, base, length, protection)
            obj.segments.append(segment)
            if protection & mmap.PROT_EXEC:
                obj.text.append((address, address + size))
            if protection & mmap.PROT_WRITE:
                self.add_roots(base, base + length)
                segment.rooted = True
            return address
        except (MemoryError, OSError, OverflowError) as error:
            # The callback API has no recoverable allocation error channel.
            os.write(2, f"JIT allocation failed: {error}\n".encode())
            os.abort()

    def _finalize_memory(self, context, error):
        for segment in self.objects[context].segments:
            if self.mprotect(segment.base, segment.size, segment.protection):
                message = os.strerror(get_errno()).encode()
                error[0] = self.lib.function("CreateMessage", P, S)(message)
                return True
        return False

    def _destroy(self, context):
        for segment in self.objects.pop(context).segments:
            if segment.rooted:
                self.remove_roots(segment.base, segment.base + segment.size)
            segment.memory.close()

    def _notify_terminating(self, _context):
        self.pending.clear()

    def attach(self, builder):
        self.lib.function("OrcLLJITBuilderSetObjectLinkingLayerCreator", None, P, LAYER_CREATOR, P)(
            builder, self.create_layer, None
        )

    def capture(self, jit):
        if self.debug:
            layer = self.lib.function("OrcLLJITGetObjTransformLayer", P, P)(jit)
            self.lib.function("OrcObjectTransformLayerSetTransform", None, P, OBJECT_TRANSFORM, P)(
                layer, self.capture_object, None
            )
