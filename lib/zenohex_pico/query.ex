defmodule ZenohexPico.Query do
  @moduledoc false

  defmodule ReplyError do
    @moduledoc """
    An error reply returned by a Zenoh query.

    The payload contains the error response and `encoding` identifies its format.
    """

    @type t :: %__MODULE__{payload: binary(), encoding: String.t()}
    defstruct payload: <<>>, encoding: "zenoh/bytes"
  end
end
