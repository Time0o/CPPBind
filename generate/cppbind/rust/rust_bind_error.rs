use std::fmt;

#[derive(Debug, Clone)]
pub struct BindError {
    pub what: String
}

impl BindError {
    pub fn new(what: String) -> Self {
        Self { what }
    }
}

impl fmt::Display for BindError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.what)
    }
}
