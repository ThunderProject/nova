import { Container, Text, Title } from '@mantine/core';
import { Illustration } from './Illustration';
import classes from './PageNotFound.module.css';

export function PageNotFound() {
    return (
        <Container className={classes.root}>
            <div className={classes.inner}>
                <Illustration className={classes.image} />
                <div className={classes.content}>
                    <Title className={classes.title}>Page Not Found</Title>
                    <Text c="dimmed" size="lg" ta="center" className={classes.description}>
                        The page you are trying to open does not exist. You may have mistyped the address, or the
                        page has been moved to another URL. If you think this is an error contact support.
                    </Text>
                </div>
            </div>
        </Container>
    );
}