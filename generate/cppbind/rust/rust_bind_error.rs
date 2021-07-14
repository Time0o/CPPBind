mod bind_error_c {
  use std::os::raw::c_char;

  #[link(name="c_bind_error")]
  extern "C" {
    pub fn bind_error_what() -> * const c_char;
    pub fn bind_error_reset();
  }
}

pub unsafe fn bind_error_what() -> * const c_char
{
    bind_error_c::bind_error_what()
}

pub unsafe fn bind_error_reset()
{
    bind_error_c::bind_error_reset();
}

#[derive(Debug, Clone)]
pub struct BindError {
    pub what: String
}

impl BindError {
    pub unsafe fn new(what_c: * const c_char) -> Self {
        let what = match CStr::from_ptr(what_c).to_str() {
            Ok(what_slice) => what_slice.to_owned(),
            Err(_)         => String::from("exception"),
        };

        Self { what }
    }
}

impl fmt::Display for BindError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.what)
    }
}
