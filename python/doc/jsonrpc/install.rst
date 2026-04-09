Install
=======

To use JSON-RPC bindings for Nexus Chat core you will need
a ``nexuschat-rpc-server`` binary which provides Nexus Chat core API over JSON-RPC
and a ``nexuschat-rpc-client`` Python package which is a JSON-RPC client that starts ``nexuschat-rpc-server`` process and uses JSON-RPC API.

`Create a virtual environment <https://docs.python.org/3/library/venv.html>`__ if you
don’t have one already and activate it::

   $ python -m venv venv
   $ . venv/bin/activate

Install ``nexuschat-rpc-server``
--------------------------------

To get ``nexuschat-rpc-server`` binary you have three options:

1. Install ``nexuschat-rpc-server`` from PyPI using ``pip install nexuschat-rpc-server``.
2. Build and install ``nexuschat-rpc-server`` from source with ``cargo install --git https://github.com/chatmail/ashlydev99/ nexuschat-rpc-server``.
3. Download prebuilt release from https://github.com/ashlydev99/core/releases and install it into ``PATH``.

Check that ``nexuschat-rpc-server`` is installed and can run::

   $ nexuschat-rpc-server --version
   1.131.4

Then install ``nexuschat-rpc-client`` with ``pip install nexuschat-rpc-client``.

Install ``nexuschat-rpc-client``
--------------------------------

To get ``nexuschat-rpc-client`` Python library you can:

1. Install ``nexuschat-rpc-client`` from PyPI using ``pip install nexuschat-rpc-client``.
2. Install ``nexuschat-rpc-client`` from source with ``pip install git+https://github.com/ashlydev99/core.git@main#subdirectory=nexuschat-rpc-client``.
