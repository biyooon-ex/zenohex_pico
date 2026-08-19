defmodule ZenohexPico.Query do
  @moduledoc false

  defmodule ReplyError do
    @moduledoc false

    @type t :: %__MODULE__{payload: binary(), encoding: String.t()}
    defstruct payload: <<>>, encoding: "zenoh/bytes"
  end
end
