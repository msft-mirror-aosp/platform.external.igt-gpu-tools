# SPDX-License-Identifier: MIT
# Copyright © 2024-2026 Intel Corporation

import abc


class DeviceInterface(abc.ABC):
    """Base class for devices (Physical and Virtual).
    Provide common operations for all devices like bind/unbind driver.
    """
    @abc.abstractmethod
    def bind_driver(self) -> None:
        """Bind driver to device."""
        raise NotImplementedError

    @abc.abstractmethod
    def unbind_driver(self) -> None:
        """Unbind driver from device."""
        raise NotImplementedError

    @abc.abstractmethod
    def reset(self) -> None:
        """Reset device (trigger FLR)."""
        raise NotImplementedError
