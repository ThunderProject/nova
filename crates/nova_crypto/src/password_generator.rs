use rand::seq::{IteratorRandom, SliceRandom};

pub struct PasswordSpecification {
    pub include_upper: bool,
    pub include_lower: bool,
    pub include_numbers: bool,
    pub include_symbols: bool
}

pub struct PasswordCharacters {
    pub upper: String,
    pub lower: String,
    pub numbers: String,
    pub symbols: String
}

pub struct PasswordGenerator {
    pw_spec: PasswordSpecification,
    pub chars: PasswordCharacters
}

impl Default for PasswordGenerator {
    fn default() -> Self {
        PasswordGenerator::new(
            PasswordSpecification {
                include_upper: true,
                include_lower: true,
                include_numbers: true,
                include_symbols: true
            }
        )
    }
}

impl PasswordGenerator {
    pub fn new(pw_spec: PasswordSpecification) -> Self {
        Self {
            pw_spec,
            chars: PasswordCharacters {
                upper: String::from("ABCDEFGHIJKLMNOPQRSTUVWXYZ"),
                lower: String::from("abcdefghijklmnopqrstuvwxyz"),
                numbers: String::from("0123456789"),
                symbols: String::from("!@#$%^&*()")
            }
        }
    }

    pub fn generate(&self, len: usize) -> Option<String> {
        if len < self.min_size() {
            return None;
        }

        let mut chars = String::new();
        let mut password = String::with_capacity(len);
        let mut rng = rand::rng();
        let spec = &self.pw_spec;

        if spec.include_upper {
            chars.push_str(&self.chars.upper);
            password.push(self.chars.upper.chars().choose(&mut rng)?);
        }

        if spec.include_lower {
            chars.push_str(&self.chars.lower);
            password.push(self.chars.lower.chars().choose(&mut rng)?);
        }

        if spec.include_numbers {
            chars.push_str(&self.chars.numbers);
            password.push(self.chars.numbers.chars().choose(&mut rng)?);
        }

        if spec.include_symbols {
            chars.push_str(&self.chars.symbols);
            password.push(self.chars.symbols.chars().choose(&mut rng)?);
        }

        for _ in password.len()..len {
            password.push(chars.chars().choose(&mut rng)?);
        }

        let mut shuffled: Vec<_> = password.chars().collect();
        shuffled.shuffle(&mut rng);

        let generated: String = shuffled.iter().collect();

        match generated.is_empty() {
            true => None,
            false => Some(generated)
        }
    }

    fn min_size(&self) -> usize {
        [
            self.pw_spec.include_upper,
            self.pw_spec.include_lower,
            self.pw_spec.include_numbers,
            self.pw_spec.include_symbols,
        ]
        .into_iter()
        .filter(|expression| *expression)
        .count()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn contains_any(str: &str, chars: &str) -> bool {
       str.chars().any(|c| chars.contains(c))
    }

    fn contains_none(str: &str, chars: &str) -> bool {
       !contains_any(str, chars)
    }

    #[test]
    fn test_length_correctness() {
        let password_generator = PasswordGenerator::default();
        let password = password_generator.generate(32);

        assert!(password.is_some());
        assert_eq!(password.unwrap().len(), 32);
    }

    #[test]
    fn test_includes_required_character_sets() {
        let password_generator = PasswordGenerator::default();
        let password_opt = password_generator.generate(20);

        assert!(password_opt.is_some());

        let password = password_opt.unwrap();

        assert!(!password.is_empty());

        assert!(contains_any(&password, &password_generator.chars.upper),   "missing uppercase");
        assert!(contains_any(&password, &password_generator.chars.lower),   "missing lowercase");
        assert!(contains_any(&password, &password_generator.chars.numbers), "missing number");
        assert!(contains_any(&password, &password_generator.chars.symbols), "missing symbol");
    }

    #[test]
    fn test_respects_disabled_character_sets() {
        let password_generator = PasswordGenerator::new(
            PasswordSpecification {
                include_upper: false,
                include_lower: true,
                include_numbers: false,
                include_symbols: true,
            }
        );

        let password_opt = password_generator.generate(20);

        assert!(password_opt.is_some());

        let password = password_opt.unwrap();
        assert!(!password.is_empty());

        assert!(contains_none(&password, &password_generator.chars.upper));
        assert!(contains_any(&password, &password_generator.chars.lower));
        assert!(contains_none(&password, &password_generator.chars.numbers));
        assert!(contains_any(&password, &password_generator.chars.symbols));
    }

    #[test]
    fn test_returns_none_when_length_too_short() {
        // Requires upper+lower+numbers+symbols = 4 characters minimum
        let password_generator = PasswordGenerator::default();
        let password_opt = password_generator.generate(3);

        assert!(password_opt.is_none());
    }

    #[test]
    fn test_randomness_sanity() {
        let password_generator = PasswordGenerator::default();
        let password_opt1 = password_generator.generate(32);
        let password_opt2 = password_generator.generate(32);

        assert!(password_opt1.is_some());
        assert!(password_opt2.is_some());

        let password1 = password_opt1.unwrap();
        let password2 = password_opt2.unwrap();

        assert!(!password1.is_empty());
        assert!(!password2.is_empty());

        assert_ne!(password1, password2);
    }

    #[test]
    fn test_configuration_is_respected() {
        let spec = PasswordSpecification {
            include_upper: false,
            include_lower: false,
            include_numbers: true,
            include_symbols: false,
        };

        let password_generator = PasswordGenerator::new(spec);
        let password_opt = password_generator.generate(10);

        assert!(password_opt.is_some());

        let password = password_opt.unwrap();
        assert!(!password.is_empty());

        assert!(contains_none(&password, &password_generator.chars.upper));
        assert!(contains_none(&password, &password_generator.chars.lower));
        assert!(contains_any(&password, &password_generator.chars.numbers));
        assert!(contains_none(&password, &password_generator.chars.symbols));
    }
}
